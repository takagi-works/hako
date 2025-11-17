# SPDX-License-Identifier: Apache-2.0
# Zephyr::GPIO Ruby extension

if(CONFIG_HAKO_ZEPHYR_GPIO)

# C binding
zephyr_library_sources(
    src/zephyr_gpio.c
)

zephyr_library_include_directories(
    include
)

# Compile Ruby sugar layer if available
if(COMMAND hako_add_ruby_library)
    file(GLOB_RECURSE GPIO_RUBY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.rb)

    if(GPIO_RUBY_SOURCES)
        hako_add_ruby_library(
            NAME zephyr_gpio
            SOURCES ${GPIO_RUBY_SOURCES}
        )

        message(STATUS "HAKO: GPIO extension Ruby sources compiled")
    endif()
endif()

endif() # CONFIG_HAKO_ZEPHYR_GPIO
