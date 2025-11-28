/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file loader.c
 * @brief HAKO bytecode loader implementation
 */

#include <hako/loader.h>
#include <hako/extension.h>
#include <mrubyc.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#ifndef CONFIG_HAKO_VM_STACK_SIZE
#define CONFIG_HAKO_VM_STACK_SIZE 4096
#endif

#ifndef CONFIG_HAKO_VM_PRIORITY
#define CONFIG_HAKO_VM_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO
#endif

#define VM_STACK_SIZE CONFIG_HAKO_VM_STACK_SIZE

LOG_MODULE_REGISTER(hako_loader, CONFIG_HAKO_LOG_LEVEL);

/* Global bytecode registry for require() support */
#define MAX_BYTECODE_MODULES 32

static struct {
    const char *name;
    const uint8_t *bytecode;
} g_bytecode_registry[MAX_BYTECODE_MODULES];

static size_t g_bytecode_count = 0;

/* VM instance */
__attribute__((aligned(8)))
static uint8_t g_memory_pool[CONFIG_HAKO_MEMORY_SIZE];
static bool g_vm_initialized = false;
static mrbc_vm *g_vm = NULL;

static K_MUTEX_DEFINE(g_vm_mutex);
K_THREAD_STACK_DEFINE(g_vm_stack, VM_STACK_SIZE);
static struct k_thread g_vm_thread;
static bool g_vm_thread_started;
static bool g_core_methods_registered;

static const uint8_t *hako_find_bytecode_locked(const char *name);
static int hako_load_bytecode_locked(const char *name, const uint8_t *bytecode);
static void hako_register_core_methods(void);
int hako_start_vm_thread(void);
void hako_vm_thread(void *p1, void *p2, void *p3);

extern struct RBuiltinClass mrbc_class_Task;
extern struct RBuiltinClass mrbc_class_Mutex;
extern struct RBuiltinClass mrbc_class_VM;

int hako_init(void)
{
    k_mutex_lock(&g_vm_mutex, K_FOREVER);
    if (g_vm_initialized) {
        LOG_WRN("HAKO VM already initialized");
        k_mutex_unlock(&g_vm_mutex);
        return 0;
    }

    /* Initialize mruby/c VM and scheduler */
    mrbc_init(g_memory_pool, CONFIG_HAKO_MEMORY_SIZE);
    hako_register_core_methods();

    g_vm_initialized = true;
    g_vm_thread_started = false;
    g_bytecode_count = 0;

    k_mutex_unlock(&g_vm_mutex);

    LOG_INF("HAKO VM initialized (memory: %d bytes)", CONFIG_HAKO_MEMORY_SIZE);

    /* Auto-discover and initialize extensions */
    //hako_init_extensions();

    return 0;
}

int hako_load_registry(const struct hako_bytecode_entry *registry, size_t count)
{
    k_mutex_lock(&g_vm_mutex, K_FOREVER);
    if (!g_vm_initialized) {
        k_mutex_unlock(&g_vm_mutex);
        LOG_ERR("VM not initialized. Call hako_init() first");
        return -EINVAL;
    }

    if (!registry) {
        k_mutex_unlock(&g_vm_mutex);
        LOG_ERR("Invalid registry pointer");
        return -EINVAL;
    }

    LOG_INF("Loading bytecode registry: %zu modules", count);

    for (size_t i = 0; i < count && registry[i].name != NULL; i++) {
        const char *name = registry[i].name;
        const uint8_t *bytecode = registry[i].bytecode;

        if (!bytecode) {
            LOG_WRN("Skipping module '%s': NULL bytecode", name);
            continue;
        }

        /* Register for require() */
        if (g_bytecode_count < MAX_BYTECODE_MODULES) {
            g_bytecode_registry[g_bytecode_count].name = name;
            g_bytecode_registry[g_bytecode_count].bytecode = bytecode;
            g_bytecode_count++;
        } else {
            k_mutex_unlock(&g_vm_mutex);
            LOG_ERR("Bytecode registry full (max %d modules)", MAX_BYTECODE_MODULES);
            return -ENOMEM;
        }

        LOG_DBG("Registered module: %s", name);
    }

    k_mutex_unlock(&g_vm_mutex);

    LOG_INF("Successfully registered %zu modules", g_bytecode_count);
    return 0;
}

int hako_load_bytecode(const char *name, const uint8_t *bytecode)
{
    int ret;

    k_mutex_lock(&g_vm_mutex, K_FOREVER);
    if (!g_vm_initialized) {
        k_mutex_unlock(&g_vm_mutex);
        LOG_ERR("VM not initialized");
        return -EINVAL;
    }

    if (!bytecode) {
        k_mutex_unlock(&g_vm_mutex);
        LOG_ERR("Invalid bytecode pointer");
        return -EINVAL;
    }

    ret = hako_load_bytecode_locked(name, bytecode);
    k_mutex_unlock(&g_vm_mutex);

    return ret;
}

int hako_run(void)
{
    return hako_start_vm_thread();
}

const uint8_t *hako_find_bytecode(const char *name)
{
    const uint8_t *bytecode;

    k_mutex_lock(&g_vm_mutex, K_FOREVER);
    bytecode = hako_find_bytecode_locked(name);
    k_mutex_unlock(&g_vm_mutex);

    return bytecode;
}

/* Extension auto-initialization */

/* Linker symbols for .hako_extensions section */
extern struct hako_extension_entry __hako_extensions_start[];
extern struct hako_extension_entry __hako_extensions_end[];

void hako_init_extensions(void)
{
    unsigned int key = irq_lock();
    struct hako_extension_entry *ext;
    size_t count = 0;

    LOG_INF("Discovering HAKO extensions...");

    for (ext = __hako_extensions_start; ext < __hako_extensions_end; ext++) {
        count++;
    }
    LOG_INF("Found %zu extension(s)", count);

    /* původní priority loop pryč */
    for (ext = __hako_extensions_start; ext < __hako_extensions_end; ext++) {
        LOG_DBG("Initializing extension: %s (priority %d)",
                ext->name, ext->priority);
        ext->init();
    }

    LOG_INF("All extensions initialized");
    irq_unlock(key);
}

mrbc_vm *hako_get_vm(void)
{
    mrbc_vm *vm;

    k_mutex_lock(&g_vm_mutex, K_FOREVER);
    vm = g_vm;
    k_mutex_unlock(&g_vm_mutex);

    return vm;
}

static const uint8_t *hako_find_bytecode_locked(const char *name)
{
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < g_bytecode_count; i++) {
        if (strcmp(g_bytecode_registry[i].name, name) == 0) {
            LOG_DBG("Found bytecode: %s", name);
            return g_bytecode_registry[i].bytecode;
        }
    }

    LOG_DBG("Bytecode not found: %s", name);
    return NULL;
}

static int hako_load_bytecode_locked(const char *name, const uint8_t *bytecode)
{
    mrbc_tcb *tcb = mrbc_create_task(bytecode, NULL);
    if (!tcb) {
        LOG_ERR("Failed to create task for %s", name ? name : "<unknown>");
        return -ENOMEM;
    }

    if (name) {
        mrbc_set_task_name(tcb, name);
    }

    if (!g_vm) {
        g_vm = &tcb->vm;
    }

    LOG_INF("Loaded bytecode: %s", name ? name : "<unknown>");
    return 0;
}

static void hako_register_core_methods(void)
{
    static const struct {
        mrbc_class *cls;
        const char *name;
    } method_table[] = {
        { MRBC_CLASS(Object), "sleep" },
        { MRBC_CLASS(Object), "sleep_ms" },
        { MRBC_CLASS(Task), "create" },
        { MRBC_CLASS(Task), "current" },
        { MRBC_CLASS(Task), "get" },
        { MRBC_CLASS(Task), "join" },
        { MRBC_CLASS(Task), "list" },
        { MRBC_CLASS(Task), "name" },
        { MRBC_CLASS(Task), "name=" },
        { MRBC_CLASS(Task), "name_list" },
        { MRBC_CLASS(Task), "pass" },
        { MRBC_CLASS(Task), "priority" },
        { MRBC_CLASS(Task), "priority=" },
        { MRBC_CLASS(Task), "raise" },
        { MRBC_CLASS(Task), "resume" },
        { MRBC_CLASS(Task), "rewind" },
        { MRBC_CLASS(Task), "run" },
        { MRBC_CLASS(Task), "status" },
        { MRBC_CLASS(Task), "suspend" },
        { MRBC_CLASS(Task), "terminate" },
        { MRBC_CLASS(Task), "value" },
        { MRBC_CLASS(Mutex), "new" },
        { MRBC_CLASS(Mutex), "lock" },
        { MRBC_CLASS(Mutex), "unlock" },
        { MRBC_CLASS(Mutex), "try_lock" },
        { MRBC_CLASS(Mutex), "locked?" },
        { MRBC_CLASS(Mutex), "owned?" },
        { MRBC_CLASS(VM), "tick" },
    };

    if (g_core_methods_registered) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(method_table); i++) {
        mrbc_method method;
        mrbc_class *cls = method_table[i].cls;

        if (!mrbc_find_method(&method, cls, mrbc_str_to_symid(method_table[i].name))) {
            LOG_WRN("Core method missing: %s", method_table[i].name);
            continue;
        }

        if (method.func) {
            mrbc_define_method(NULL, cls, method_table[i].name, method.func);
        }
    }

    g_core_methods_registered = true;
}

void hako_vm_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    uint32_t iter = 0;

    while (1) {
        mrbc_run();
        mrbc_tick();
        k_msleep(1);
    }
}

int hako_start_vm_thread(void)
{
    int ret = 0;

    k_mutex_lock(&g_vm_mutex, K_FOREVER);

    if (!g_vm_initialized) {
        k_mutex_unlock(&g_vm_mutex);
        LOG_ERR("VM not initialized");
        return -EINVAL;
    }

    if (g_vm_thread_started) {
        k_mutex_unlock(&g_vm_mutex);
        return 0;
    }

    const uint8_t *main_bytecode = hako_find_bytecode_locked("main");
    if (main_bytecode) {
        ret = hako_load_bytecode_locked("main", main_bytecode);
        if (ret < 0) {
            k_mutex_unlock(&g_vm_mutex);
            return ret;
        }
    } else {
        LOG_WRN("Main bytecode not found; VM thread will idle until tasks are created");
    }

    k_thread_create(&g_vm_thread, g_vm_stack, VM_STACK_SIZE,
                    hako_vm_thread, NULL, NULL, NULL,
                    CONFIG_HAKO_VM_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&g_vm_thread, "hako_vm");

    g_vm_thread_started = true;

    k_mutex_unlock(&g_vm_mutex);
    return ret;
}
