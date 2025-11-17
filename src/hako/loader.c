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

LOG_MODULE_REGISTER(hako_loader, CONFIG_HAKO_LOG_LEVEL);

/* Global bytecode registry for require() support */
#define MAX_BYTECODE_MODULES 32

static struct {
    const char *name;
    const uint8_t *bytecode;
} g_bytecode_registry[MAX_BYTECODE_MODULES];

static size_t g_bytecode_count = 0;

/* VM instance */
static uint8_t g_memory_pool[CONFIG_HAKO_MEMORY_SIZE];
static bool g_vm_initialized = false;
static mrbc_vm *g_vm = NULL;

int hako_init(void)
{
    if (g_vm_initialized) {
        LOG_WRN("HAKO VM already initialized");
        return 0;
    }

    /* Initialize mruby/c VM */
    mrbc_init_alloc(g_memory_pool, CONFIG_HAKO_MEMORY_SIZE);
    mrbc_init_global();
    mrbc_init_class();

    /* Create global VM instance */
    g_vm = mrbc_vm_open(NULL);
    if (!g_vm) {
        LOG_ERR("Failed to create VM instance");
        return -ENOMEM;
    }

    g_vm_initialized = true;
    g_bytecode_count = 0;

    LOG_INF("HAKO VM initialized (memory: %d bytes)", CONFIG_HAKO_MEMORY_SIZE);

    /* Auto-discover and initialize extensions */
    hako_init_extensions();

    return 0;
}

int hako_load_registry(const struct hako_bytecode_entry *registry, size_t count)
{
    if (!g_vm_initialized) {
        LOG_ERR("VM not initialized. Call hako_init() first");
        return -EINVAL;
    }

    if (!registry) {
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
            LOG_ERR("Bytecode registry full (max %d modules)", MAX_BYTECODE_MODULES);
            return -ENOMEM;
        }

        LOG_DBG("Registered module: %s", name);
    }

    LOG_INF("Successfully registered %zu modules", g_bytecode_count);
    return 0;
}

int hako_load_bytecode(const char *name, const uint8_t *bytecode)
{
    if (!g_vm_initialized || !g_vm) {
        LOG_ERR("VM not initialized");
        return -EINVAL;
    }

    if (!bytecode) {
        LOG_ERR("Invalid bytecode pointer");
        return -EINVAL;
    }

    /* Load bytecode into global VM */
    if (mrbc_load_mrb(g_vm, bytecode) != 0) {
        LOG_ERR("Failed to load bytecode: %s", name ? name : "<unknown>");
        return -EIO;
    }

    LOG_INF("Loaded bytecode: %s", name ? name : "<unknown>");
    return 0;
}

int hako_run(void)
{
    if (!g_vm_initialized || !g_vm) {
        LOG_ERR("VM not initialized");
        return -EINVAL;
    }

    LOG_INF("Starting Ruby VM...");

    /* Run the global VM */
    mrbc_vm_begin(g_vm);
    mrbc_vm_run(g_vm);
    mrbc_vm_end(g_vm);

    LOG_INF("Ruby VM finished");
    return 0;
}

const uint8_t *hako_find_bytecode(const char *name)
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

/* Extension auto-initialization */

/* Linker symbols for .hako_extensions section */
extern struct hako_extension_entry __hako_extensions_start[];
extern struct hako_extension_entry __hako_extensions_end[];

void hako_init_extensions(void)
{
    struct hako_extension_entry *ext;
    size_t count = 0;

    LOG_INF("Discovering HAKO extensions...");

    /* Count and sort extensions by priority (simple bubble sort) */
    for (ext = __hako_extensions_start; ext < __hako_extensions_end; ext++) {
        count++;
    }

    if (count == 0) {
        LOG_INF("No extensions found");
        return;
    }

    LOG_INF("Found %zu extension(s)", count);

    /* Initialize extensions in priority order */
    for (uint8_t prio = 0; prio <= 255; prio++) {
        for (ext = __hako_extensions_start; ext < __hako_extensions_end; ext++) {
            if (ext->priority == prio && ext->init) {
                LOG_DBG("Initializing extension: %s (priority %d)",
                        ext->name, ext->priority);
                ext->init();
            }
        }
    }

    LOG_INF("All extensions initialized");
}
