/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file extension.h
 * @brief HAKO extension auto-registration mechanism
 *
 * Extensions use HAKO_EXTENSION_DEFINE() macro to automatically register
 * themselves with the HAKO loader. No manual initialization needed!
 */

#ifndef HAKO_EXTENSION_H
#define HAKO_EXTENSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extension entry in the registry
 */
struct hako_extension_entry {
    const char *name;           /**< Extension name */
    void (*init)(void);         /**< Initialization function */
    uint8_t priority;           /**< Init priority (lower = earlier) */
};

/**
 * @brief Define a HAKO extension with auto-registration
 *
 * This macro places the extension entry in a special linker section
 * (.hako_extensions) that is automatically discovered and initialized
 * by the HAKO loader.
 *
 * @param ext_name Extension identifier (e.g., zephyr_gpio)
 * @param init_fn Initialization function (void (*)(void))
 * @param prio Priority (0-255, lower runs first)
 *
 * Example:
 * @code
 * static void my_extension_init(void) {
 *     // Setup extension...
 * }
 *
 * HAKO_EXTENSION_DEFINE(my_extension, my_extension_init, 50);
 * @endcode
 */
#define HAKO_EXTENSION_DEFINE(ext_name, init_fn, prio) \
    static const struct hako_extension_entry __hako_ext_##ext_name \
    __attribute__((__section__(".hako_extensions"))) \
    __attribute__((__used__)) = { \
        .name = #ext_name, \
        .init = init_fn, \
        .priority = prio \
    }

/**
 * @brief Default priority for most extensions
 */
#define HAKO_EXTENSION_PRIORITY_DEFAULT 50

/**
 * @brief Early init priority (core extensions)
 */
#define HAKO_EXTENSION_PRIORITY_EARLY 10

/**
 * @brief Late init priority (app-level extensions)
 */
#define HAKO_EXTENSION_PRIORITY_LATE 90

/**
 * @brief Initialize all registered HAKO extensions
 *
 * Called automatically by hako_init().
 * Walks through .hako_extensions linker section and calls init functions
 * in priority order.
 */
void hako_init_extensions(void);

#ifdef __cplusplus
}
#endif

#endif /* HAKO_EXTENSION_H */
