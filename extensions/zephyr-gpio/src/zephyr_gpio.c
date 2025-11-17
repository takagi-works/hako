/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file zephyr_gpio.c
 * @brief Zephyr::GPIO Ruby extension
 */

#include <hako/extension.h>
#include <mrubyc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_gpio, CONFIG_HAKO_LOG_LEVEL);

// Simplified GPIO handle (stores only pin number for demo)
// In production, should store full gpio_dt_spec
typedef struct {
    int pin;
    int flags;
} gpio_handle_t;

/**
 * Zephyr::GPIO.open(pin_number, mode: :output)
 *
 * Simplified version - just stores pin number
 * Production version would resolve devicetree aliases
 */
static void c_gpio_open(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc < 1) {
        mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
        return;
    }

    // Get pin number (simplified - should handle symbols for aliases)
    int pin = mrbc_integer(v[1]);

    LOG_DBG("GPIO.open(pin=%d)", pin);

    // Create instance with handle
    mrbc_value obj = mrbc_instance_new(vm, v[0].cls, sizeof(gpio_handle_t));
    gpio_handle_t *handle = (gpio_handle_t *)obj.instance->data;
    handle->pin = pin;
    handle->flags = 0; // GPIO_OUTPUT simplified

    SET_RETURN(obj);
}

/**
 * gpio.write(value)
 */
static void c_gpio_write(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc != 1) {
        mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
        return;
    }

    gpio_handle_t *handle = (gpio_handle_t *)v[0].instance->data;
    int value = mrbc_integer(v[1]);

    LOG_DBG("GPIO.write(pin=%d, value=%d)", handle->pin, value);

    // TODO: Actual GPIO write via Zephyr API
    // gpio_pin_set_raw(handle->spec.port, handle->spec.pin, value);
}

/**
 * gpio.read() -> Integer
 */
static void c_gpio_read(mrbc_vm *vm, mrbc_value *v, int argc)
{
    gpio_handle_t *handle = (gpio_handle_t *)v[0].instance->data;

    // TODO: Actual GPIO read
    // int value = gpio_pin_get_raw(handle->spec.port, handle->spec.pin);
    int value = 0; // Placeholder

    LOG_DBG("GPIO.read(pin=%d) -> %d", handle->pin, value);

    SET_INT_RETURN(value);
}

/**
 * gpio.toggle()
 */
static void c_gpio_toggle(mrbc_vm *vm, mrbc_value *v, int argc)
{
    gpio_handle_t *handle = (gpio_handle_t *)v[0].instance->data;

    LOG_DBG("GPIO.toggle(pin=%d)", handle->pin);

    // TODO: Actual GPIO toggle
    // gpio_pin_toggle(handle->spec.port, handle->spec.pin);
}

/**
 * Initialize Zephyr::GPIO extension
 */
static void zephyr_gpio_init(void)
{
    LOG_INF("Initializing Zephyr::GPIO extension");

    // Create or get Zephyr module
    mrbc_class *zephyr_mod = mrbc_define_module(0, "Zephyr");

    // Create GPIO class under Zephyr module
    mrbc_class *gpio_cls = mrbc_define_class_under(0, zephyr_mod, "GPIO",
                                                    mrbc_class_object);

    // Class methods (singleton methods on GPIO class)
    mrbc_define_method(0, gpio_cls, "open", c_gpio_open);

    // Instance methods
    mrbc_define_method(0, gpio_cls, "write", c_gpio_write);
    mrbc_define_method(0, gpio_cls, "read", c_gpio_read);
    mrbc_define_method(0, gpio_cls, "toggle", c_gpio_toggle);

    LOG_INF("Zephyr::GPIO extension initialized");
}

/* Auto-register extension - no manual init needed! */
HAKO_EXTENSION_DEFINE(zephyr_gpio, zephyr_gpio_init,
                      HAKO_EXTENSION_PRIORITY_DEFAULT);
