# SPDX-License-Identifier: Apache-2.0
# HAKO Extensions

# Each extension is conditionally included based on its Kconfig option
# Extensions auto-register using HAKO_EXTENSION_DEFINE() macro

# GPIO extension
if(CONFIG_HAKO_ZEPHYR_GPIO)
    add_subdirectory(zephyr-gpio)
endif()

# Add more extensions here as they're created:
# if(CONFIG_HAKO_ZEPHYR_I2C)
#     add_subdirectory(zephyr-i2c)
# endif()
