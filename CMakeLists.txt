# SPDX-License-Identifier: Apache-2.0
# Hako library for Zephyr

# Include HAKO CMake utilities (available to all projects)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/hako.cmake)

if(CONFIG_HAKO)

zephyr_library()

# Core Hako VM sources (mrubyc)
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

# HAKO loader (bytecode loading utilities)
zephyr_library_sources(
  src/hako/loader.c
)

# Add linker script for extension sections
zephyr_linker_sources(SECTIONS ${CMAKE_CURRENT_LIST_DIR}/include/linker/hako-sections.ld)

# Include directories for the library itself
zephyr_library_include_directories(
  ext/mrubyc/src
  ext/mrubyc/hal/zephyr
  include
)

# Expose headers to applications
zephyr_include_directories(
  ext/mrubyc/src
  ext/mrubyc/hal/zephyr
  include
)

# Optional: Add compiler definitions
# zephyr_library_compile_definitions(
#   MRBC_USE_MATH=1
# )

# PicoRuby Compiler support (mruby-compiler2 + Prism)
if(CONFIG_HAKO_COMPILER)

  # Compiler core sources
  zephyr_library_sources(
    ext/picoruby/mruby-compiler2/src/ccontext.c
    ext/picoruby/mruby-compiler2/src/cdump.c
    ext/picoruby/mruby-compiler2/src/codedump.c
    ext/picoruby/mruby-compiler2/src/codegen.c
    ext/picoruby/mruby-compiler2/src/compile.c
    ext/picoruby/mruby-compiler2/src/debug.c
    ext/picoruby/mruby-compiler2/src/diagnostic.c
    ext/picoruby/mruby-compiler2/src/dump.c
    ext/picoruby/mruby-compiler2/src/irep.c
    ext/picoruby/mruby-compiler2/src/mrc_presym.c
    ext/picoruby/mruby-compiler2/src/parser_util.c
    ext/picoruby/mruby-compiler2/src/pool.c
  )

  # Prism parser sources
  zephyr_library_sources(
    ext/picoruby/mruby-compiler2/lib/prism/src/diagnostic.c
    ext/picoruby/mruby-compiler2/lib/prism/src/encoding.c
    ext/picoruby/mruby-compiler2/lib/prism/src/node.c
    ext/picoruby/mruby-compiler2/lib/prism/src/options.c
    ext/picoruby/mruby-compiler2/lib/prism/src/pack.c
    ext/picoruby/mruby-compiler2/lib/prism/src/prettyprint.c
    ext/picoruby/mruby-compiler2/lib/prism/src/prism.c
    ext/picoruby/mruby-compiler2/lib/prism/src/regexp.c
    ext/picoruby/mruby-compiler2/lib/prism/src/serialize.c
    ext/picoruby/mruby-compiler2/lib/prism/src/static_literals.c
    ext/picoruby/mruby-compiler2/lib/prism/src/token_type.c
    # Prism utility sources
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_buffer.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_char.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_constant_pool.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_integer.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_list.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_memchr.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_newline_list.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_string.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_strncasecmp.c
    ext/picoruby/mruby-compiler2/lib/prism/src/util/pm_strpbrk.c
  )

  # Compiler include directories
  zephyr_library_include_directories(
    ext/picoruby/mruby-compiler2/include
    ext/picoruby/mruby-compiler2/lib/prism/include
  )

  # Expose compiler headers to applications
  zephyr_include_directories(
    ext/picoruby/mruby-compiler2/include
    ext/picoruby/mruby-compiler2/lib/prism/include
  )

  # Compiler definitions
  zephyr_library_compile_definitions(
    MRC_TARGET_HAKO=1
    PRISM_EXPORT_SYMBOLS=1
  )

  # Size optimization
  if(CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE)
    zephyr_library_compile_definitions(
      PRISM_BUILD_MINIMAL=1
    )
  endif()

  # Debug mode
  if(CONFIG_HAKO_COMPILER_DEBUG)
    zephyr_library_compile_definitions(
      MRC_DEBUG=1
    )
  endif()

  # picoruby-eval support
  if(CONFIG_HAKO_EVAL)
    zephyr_library_sources(
      ext/picoruby/picoruby-eval/src/mrubyc/eval.c
    )
  endif()

  # Zephyr shell IRB integration
  if(CONFIG_HAKO_IRB_COMMAND)
    zephyr_library_sources(
      zephyr/shell_irb.c
      zephyr/file_zephyr.c
    )
  endif()

endif() # CONFIG_HAKO_COMPILER

# =============================================================================
# PicoRuby Standard Library Gems
# =============================================================================

# picoruby-require (require/load support)
if(CONFIG_HAKO_REQUIRE)
  zephyr_library_sources(
    ext/picoruby/picoruby-require/src/mrubyc/require.c
  )
  zephyr_library_include_directories(
    ext/picoruby/picoruby-require/include
  )
endif()

# picoruby-sandbox (isolated VM execution)
if(CONFIG_HAKO_SANDBOX)
  zephyr_library_sources(
    # Only compile sandbox.c which includes mrubyc/sandbox.c
    ext/picoruby/picoruby-sandbox/src/sandbox.c
  )
  zephyr_library_compile_definitions(
    PICORB_VM_MRUBYC=1
  )
  # Note: picoruby-sandbox doesn't have a separate include directory
endif()

# picoruby-metaprog (metaprogramming/reflection)
if(CONFIG_HAKO_METAPROG)
  zephyr_library_sources(
    ext/picoruby/picoruby-metaprog/src/mrubyc/metaprog.c
  )
endif()

# picoruby-posix-io (File/IO with POSIX)
if(CONFIG_HAKO_POSIX_IO)
  zephyr_library_sources(
    ext/picoruby/picoruby-posix-io/src/io.c
    ext/picoruby/picoruby-posix-io/src/file.c
    ext/picoruby/picoruby-posix-io/src/file_test.c
  )
  zephyr_library_include_directories(
    ext/picoruby/picoruby-posix-io/include
  )
  # POSIX API is required and auto-selected by Kconfig
endif()

# =============================================================================
# HAKO Extensions (auto-registered)
# =============================================================================

# Load all extensions
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/extensions)
    add_subdirectory(extensions)
endif()

endif() # CONFIG_HAKO

