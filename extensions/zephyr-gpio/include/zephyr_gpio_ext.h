/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file zephyr_gpio_ext.h
 * @brief Zephyr::GPIO extension public API
 */

#ifndef ZEPHYR_GPIO_EXT_H
#define ZEPHYR_GPIO_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Zephyr::GPIO Ruby extension
 *
 * Registers the Zephyr::GPIO class and its methods with mruby/c VM.
 * Called automatically by HAKO loader during initialization.
 */
void mrbc_zephyr_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_GPIO_EXT_H */
