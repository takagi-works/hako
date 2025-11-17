/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file loader.h
 * @brief HAKO bytecode loader for embedded Ruby applications
 */

#ifndef HAKO_LOADER_H
#define HAKO_LOADER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bytecode registry entry
 *
 * Represents a single Ruby bytecode module that's embedded in firmware.
 */
struct hako_bytecode_entry {
    const char *name;           /**< Module name (without .rb extension) */
    const uint8_t *bytecode;    /**< Pointer to mruby bytecode array */
};

/**
 * @brief Initialize HAKO VM
 *
 * Initializes the mruby/c virtual machine and allocates necessary resources.
 *
 * @return 0 on success, negative error code on failure
 */
int hako_init(void);

/**
 * @brief Load a bytecode registry into VM
 *
 * Loads all bytecode modules from a registry into the VM. This allows
 * require() to find these modules at runtime.
 *
 * @param registry Pointer to bytecode registry array (NULL-terminated)
 * @param count Number of entries in registry (excluding NULL terminator)
 * @return 0 on success, negative error code on failure
 */
int hako_load_registry(const struct hako_bytecode_entry *registry, size_t count);

/**
 * @brief Load single bytecode module
 *
 * @param name Module name (for debugging/logging)
 * @param bytecode Pointer to mruby bytecode array
 * @return 0 on success, negative error code on failure
 */
int hako_load_bytecode(const char *name, const uint8_t *bytecode);

/**
 * @brief Run the Ruby VM
 *
 * Starts executing loaded Ruby bytecode. This function blocks until
 * the Ruby program completes or an error occurs.
 *
 * @return 0 on success, negative error code on failure
 */
int hako_run(void);

/**
 * @brief Find bytecode by module name
 *
 * Searches registered bytecode modules for a given name. Used internally
 * by require() implementation.
 *
 * @param name Module name to find
 * @return Pointer to bytecode if found, NULL otherwise
 */
const uint8_t *hako_find_bytecode(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* HAKO_LOADER_H */
