# SPDX-License-Identifier: BSD-3-Clause
# mruby/c library for Zephyr

if(CONFIG_MRUBYC)

zephyr_library()

# Core mruby/c sources
zephyr_library_sources(
  ext/mrubyc/src/alloc.c
  ext/mrubyc/src/c_array.c
  ext/mrubyc/src/c_hash.c
  ext/mrubyc/src/c_math.c
  ext/mrubyc/src/c_numeric.c
  ext/mrubyc/src/c_object.c
  ext/mrubyc/src/c_proc.c
  ext/mrubyc/src/c_range.c
  ext/mrubyc/src/c_string.c
  ext/mrubyc/src/class.c
  ext/mrubyc/src/console.c
  ext/mrubyc/src/error.c
  ext/mrubyc/src/global.c
  ext/mrubyc/src/keyvalue.c
  ext/mrubyc/src/load.c
  ext/mrubyc/src/mrblib.c
  ext/mrubyc/src/rrt0.c
  ext/mrubyc/src/symbol.c
  ext/mrubyc/src/value.c
  ext/mrubyc/src/vm.c
)

# HAL for Zephyr
zephyr_library_sources(
  ext/mrubyc/hal/zephyr/hal.c
)

# Include directories for the library itself
zephyr_library_include_directories(
  ext/mrubyc/src
  ext/mrubyc/hal/zephyr
)

# Expose headers to applications
zephyr_include_directories(
  ext/mrubyc/src
  ext/mrubyc/hal/zephyr
)

# Optional: Add compiler definitions
# zephyr_library_compile_definitions(
#   MRBC_USE_MATH=1
# )

endif() # CONFIG_MRUBYC

