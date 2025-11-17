# SPDX-License-Identifier: Apache-2.0
# HAKO CMake utilities for Ruby bytecode compilation

# Find mrbc compiler
function(hako_find_mrbc)
    if(NOT MRBC_EXECUTABLE)
        find_program(MRBC_EXECUTABLE
            NAMES mrbc
            PATHS
                $ENV{HOME}/.local/bin
                /tmp/mruby/build/host/bin
                /usr/local/bin
                ${HAKO_MRBC_PATH}
            DOC "mruby bytecode compiler"
        )

        if(NOT MRBC_EXECUTABLE)
            message(WARNING
                "mrbc compiler not found. Ruby compilation will be skipped.\n"
                "Install mruby or set HAKO_MRBC_PATH environment variable."
            )
            set(MRBC_EXECUTABLE "" PARENT_SCOPE)
            return()
        endif()

        message(STATUS "HAKO: Found mrbc at ${MRBC_EXECUTABLE}")
        set(MRBC_EXECUTABLE ${MRBC_EXECUTABLE} CACHE FILEPATH "Path to mrbc compiler")
    endif()
endfunction()

# Compile single .rb file to C array
# Usage: hako_compile_ruby_to_c(
#            RUBY_FILE path/to/file.rb
#            OUTPUT_FILE path/to/output.c
#            SYMBOL_NAME symbol_name
#        )
function(hako_compile_ruby_to_c)
    set(oneValueArgs RUBY_FILE OUTPUT_FILE SYMBOL_NAME)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT ARG_RUBY_FILE)
        message(FATAL_ERROR "RUBY_FILE is required")
    endif()

    if(NOT ARG_OUTPUT_FILE)
        get_filename_component(rb_name ${ARG_RUBY_FILE} NAME_WE)
        set(ARG_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/hako_bytecode/${rb_name}.c")
    endif()

    if(NOT ARG_SYMBOL_NAME)
        get_filename_component(rb_name ${ARG_RUBY_FILE} NAME_WE)
        set(ARG_SYMBOL_NAME "mrb_bytecode_${rb_name}")
    endif()

    # Ensure mrbc is available
    hako_find_mrbc()

    if(NOT MRBC_EXECUTABLE)
        message(WARNING "Skipping Ruby compilation: ${ARG_RUBY_FILE}")
        return()
    endif()

    # Create output directory
    get_filename_component(output_dir ${ARG_OUTPUT_FILE} DIRECTORY)

    # Compile .rb -> .c with bytecode array
    add_custom_command(
        OUTPUT ${ARG_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
        COMMAND ${MRBC_EXECUTABLE} -B${ARG_SYMBOL_NAME} -o ${ARG_OUTPUT_FILE} ${ARG_RUBY_FILE}
        DEPENDS ${ARG_RUBY_FILE}
        COMMENT "HAKO: Compiling ${ARG_RUBY_FILE} -> ${ARG_SYMBOL_NAME}"
        VERBATIM
    )

    # Return output file path to parent scope
    set(HAKO_COMPILED_C_FILE ${ARG_OUTPUT_FILE} PARENT_SCOPE)
endfunction()

# Add Ruby library - compiles all .rb files to C arrays and links them
# Usage: hako_add_ruby_library(
#            NAME library_name
#            SOURCES file1.rb file2.rb ...
#            [TARGET target_name]  # Optional, defaults to 'app'
#        )
function(hako_add_ruby_library)
    set(oneValueArgs NAME TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME)
        message(FATAL_ERROR "NAME is required")
    endif()

    if(NOT ARG_SOURCES)
        message(WARNING "No Ruby sources provided for ${ARG_NAME}")
        return()
    endif()

    if(NOT ARG_TARGET)
        set(ARG_TARGET app)
    endif()

    # Ensure mrbc is available
    hako_find_mrbc()

    if(NOT MRBC_EXECUTABLE)
        message(WARNING "Skipping Ruby library ${ARG_NAME}: mrbc not found")
        return()
    endif()

    set(bytecode_c_files "")
    set(bytecode_symbols "")

    # Compile each .rb file
    foreach(rb_file ${ARG_SOURCES})
        get_filename_component(rb_name ${rb_file} NAME_WE)
        get_filename_component(rb_path ${rb_file} DIRECTORY)

        # Create unique symbol name
        string(REPLACE "/" "_" rb_path_clean "${rb_path}")
        string(REPLACE "." "_" rb_path_clean "${rb_path_clean}")
        set(symbol_name "mrb_${ARG_NAME}_${rb_name}")

        set(output_c "${CMAKE_CURRENT_BINARY_DIR}/hako_bytecode/${ARG_NAME}/${rb_name}.c")

        hako_compile_ruby_to_c(
            RUBY_FILE ${rb_file}
            OUTPUT_FILE ${output_c}
            SYMBOL_NAME ${symbol_name}
        )

        if(HAKO_COMPILED_C_FILE)
            list(APPEND bytecode_c_files ${HAKO_COMPILED_C_FILE})
            list(APPEND bytecode_symbols ${symbol_name})
        endif()
    endforeach()

    if(NOT bytecode_c_files)
        message(WARNING "No bytecode files generated for ${ARG_NAME}")
        return()
    endif()

    # Generate registry file
    set(registry_file "${CMAKE_CURRENT_BINARY_DIR}/hako_bytecode/${ARG_NAME}_registry.c")
    set(registry_header "${CMAKE_CURRENT_BINARY_DIR}/hako_bytecode/${ARG_NAME}_registry.h")

    # Generate header
    file(WRITE ${registry_header} "// Auto-generated HAKO bytecode registry for ${ARG_NAME}\n")
    file(APPEND ${registry_header} "#ifndef HAKO_${ARG_NAME}_REGISTRY_H\n")
    file(APPEND ${registry_header} "#define HAKO_${ARG_NAME}_REGISTRY_H\n\n")
    file(APPEND ${registry_header} "#include <hako/loader.h>\n\n")
    file(APPEND ${registry_header} "extern const struct hako_bytecode_entry hako_${ARG_NAME}_registry[];\n")
    file(APPEND ${registry_header} "extern const size_t hako_${ARG_NAME}_registry_count;\n\n")
    file(APPEND ${registry_header} "#endif\n")

    # Generate source
    file(WRITE ${registry_file} "// Auto-generated HAKO bytecode registry for ${ARG_NAME}\n")
    file(APPEND ${registry_file} "#include \"${ARG_NAME}_registry.h\"\n\n")

    # Declare extern symbols
    foreach(symbol ${bytecode_symbols})
        file(APPEND ${registry_file} "extern const uint8_t ${symbol}[];\n")
    endforeach()

    # Create registry table
    file(APPEND ${registry_file} "\nconst struct hako_bytecode_entry hako_${ARG_NAME}_registry[] = {\n")

    set(entry_count 0)
    foreach(rb_file symbol IN ZIP_LISTS ARG_SOURCES bytecode_symbols)
        get_filename_component(rb_name ${rb_file} NAME_WE)
        file(APPEND ${registry_file} "    {\"${rb_name}\", ${symbol}},\n")
        math(EXPR entry_count "${entry_count} + 1")
    endforeach()

    file(APPEND ${registry_file} "    {NULL, NULL}\n")
    file(APPEND ${registry_file} "};\n\n")
    file(APPEND ${registry_file} "const size_t hako_${ARG_NAME}_registry_count = ${entry_count};\n")

    # Add all generated C files to target
    list(APPEND bytecode_c_files ${registry_file})

    target_sources(${ARG_TARGET} PRIVATE ${bytecode_c_files})
    target_include_directories(${ARG_TARGET} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/hako_bytecode)

    message(STATUS "HAKO: Added Ruby library '${ARG_NAME}' with ${CMAKE_MATCH_COUNT} modules")

    # Export info for parent scope
    set(HAKO_${ARG_NAME}_BYTECODE_FILES ${bytecode_c_files} PARENT_SCOPE)
    set(HAKO_${ARG_NAME}_REGISTRY_HEADER ${registry_header} PARENT_SCOPE)
endfunction()

# Auto-discover and compile Ruby sources in standard locations
# Looks for Ruby files in src/ruby/ and lib/ directories
function(hako_auto_add_ruby)
    set(ruby_sources "")

    # Check src/ruby/
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/ruby)
        file(GLOB_RECURSE src_ruby_files ${CMAKE_CURRENT_SOURCE_DIR}/src/ruby/*.rb)
        list(APPEND ruby_sources ${src_ruby_files})
    endif()

    # Check lib/
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib)
        file(GLOB_RECURSE lib_ruby_files ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.rb)
        list(APPEND ruby_sources ${lib_ruby_files})
    endif()

    if(ruby_sources)
        list(LENGTH ruby_sources ruby_count)
        get_filename_component(project_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        message(STATUS "HAKO: Auto-discovered ${ruby_count} Ruby files in ${project_name}")

        hako_add_ruby_library(
            NAME ${project_name}
            SOURCES ${ruby_sources}
        )
    else()
        message(STATUS "HAKO: No Ruby files found in ${CMAKE_CURRENT_SOURCE_DIR}/src/ruby or ${CMAKE_CURRENT_SOURCE_DIR}/lib")
    endif()
endfunction()
