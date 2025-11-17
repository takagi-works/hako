# Hako Module Architecture

This document describes the architecture of the Hako module for Zephyr RTOS with integrated PicoRuby compiler.

## Overview

The Hako module combines multiple Ruby implementations to provide on-device compilation and execution of Ruby code on resource-constrained embedded systems:

```
┌─────────────────────────────────────────────────────────────┐
│                    User Application                         │
│  (Shell commands, C API calls, Ruby scripts)                │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────┴─────────────────────────────────────┐
│                  Hako Zephyr Module                          │
├──────────────────────────────────────────────────────────────┤
│  Shell Integration Layer (shell_irb.c)                       │
│    - ruby <expr>    : Execute Ruby expression                │
│    - ruby_file <path> : Load and execute from file           │
│    - ruby_info      : Show compiler information              │
├──────────────────────────────────────────────────────────────┤
│  Compiler Subsystem (Optional, ~500KB ROM)                   │
│  ┌────────────────────────────────────────────────────┐      │
│  │  PicoRuby Compiler (mruby-compiler2)               │      │
│  │  - Compile Ruby source → mruby bytecode            │      │
│  │  - Uses separate memory pool (65KB default)        │      │
│  │  ┌──────────────────────────────────────────┐      │      │
│  │  │  Prism Parser                            │      │      │
│  │  │  - Universal Ruby parser                 │      │      │
│  │  │  - AST generation                        │      │      │
│  │  │  - ~200-400KB ROM                        │      │      │
│  │  └──────────────────────────────────────────┘      │      │
│  └────────────────────────────────────────────────────┘      │
├──────────────────────────────────────────────────────────────┤
│  mruby/c VM (Always Present, ~40KB ROM)                      │
│    - Execute mruby bytecode                                  │
│    - Memory pool: 40KB default                               │
│    - Minimal, deterministic execution                        │
├──────────────────────────────────────────────────────────────┤
│  Zephyr Integration                                          │
│    - HAL layer (memory, timing, console)                     │
│    - Shell subsystem integration                             │
│    - Filesystem integration (optional)                       │
└──────────────────────────────────────────────────────────────┘
```

## Components

### 1. mruby/c VM (Core)

**Source**: `ext/mrubyc/`
**Size**: ~40 KB ROM, 40 KB RAM (configurable)
**Purpose**: Lightweight Ruby bytecode interpreter

The Hako VM is always present and provides:
- Bytecode execution engine
- Object system (classes, methods, inheritance)
- Garbage collection
- Core Ruby classes (String, Array, Hash, Integer, etc.)

**Key characteristics**:
- Deterministic execution (no JIT)
- Small memory footprint
- Designed for microcontrollers
- No dynamic code loading (without compiler)

### 2. PicoRuby Compiler (Optional)

**Source**: `ext/picoruby/mruby-compiler2/`
**Size**: ~250 KB ROM, 65 KB RAM (during compilation only)
**Purpose**: On-device Ruby source → bytecode compilation

The compiler enables:
- Runtime compilation of Ruby source code
- Interactive Ruby execution (REPL-like)
- Loading Ruby scripts from strings or files
- `eval()` function support

**Compilation flow**:
```
Ruby Source Code
      ↓
Prism Parser
      ↓
Abstract Syntax Tree (AST)
      ↓
mruby-compiler2 Code Generator
      ↓
Intermediate Representation (IR)
      ↓
Bytecode Dump
      ↓
mruby Bytecode
      ↓
mruby/c VM Execution
```

### 3. Prism Parser

**Source**: `ext/picoruby/mruby-compiler2/lib/prism/`
**Size**: ~200-400 KB ROM (depending on optimization)
**Purpose**: Universal Ruby parser

Features:
- C99 implementation
- Full Ruby 3.x syntax support
- Error-tolerant parsing
- Generates AST for compiler

### 4. Zephyr Integration Layer

**Source**: `zephyr/`
**Components**:
- `hal.c` - Hardware abstraction (memory, console, timing)
- `shell_irb.c` - Shell command integration
- `Kconfig` - Configuration options
- `CMakeLists.txt` - Build system integration

## Memory Architecture

### Memory Pools

The module uses separate memory pools to optimize resource usage:

```
┌─────────────────────────────────────────────────────────┐
│                      Zephyr Heap                        │
│                   (CONFIG_HEAP_MEM_POOL_SIZE)           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌────────────────────────────────────────────┐         │
│  │  mruby/c VM Pool (Always Allocated)        │         │
│  │  - Size: 40 KB default                     │         │
│  │  - Used for: Ruby objects, VM state        │         │
│  │  - Lifetime: Program lifetime              │         │
│  └────────────────────────────────────────────┘         │
│                                                         │
│  ┌────────────────────────────────────────────┐         │
│  │  Compiler Pool (Allocated During Compile)  │         │
│  │  - Size: 65 KB default                     │         │
│  │  - Used for: AST, IR, compilation state    │         │
│  │  - Lifetime: Compilation only              │         │
│  │  - Freed after bytecode generation         │         │
│  └────────────────────────────────────────────┘         │
│                                                         │
│  ┌────────────────────────────────────────────┐         │
│  │  File Buffer (When Loading Files)          │         │
│  │  - Size: 16 KB max per file                │         │
│  │  - Used for: Reading Ruby scripts          │         │
│  │  - Lifetime: File load operation only      │         │
│  └────────────────────────────────────────────┘         │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Memory Usage by Configuration

| Configuration | ROM | RAM (Static) | RAM (Peak) |
|--------------|-----|--------------|------------|
| VM Only (no compiler) | ~40 KB | 40 KB | 40 KB |
| VM + Compiler (size-optimized) | ~500 KB | 40 KB | 105 KB |
| VM + Compiler (full) | ~700 KB | 40 KB | 105 KB |
| VM + Compiler + File Support | ~500 KB | 40 KB | 121 KB |

**Notes**:
- ROM includes all code and read-only data
- RAM (Static) is always allocated
- RAM (Peak) is maximum during compilation
- File support adds 16 KB buffer during file operations

## Execution Flow

### Direct Expression Execution

```
User: ruby "puts 'Hello'"
         ↓
shell_irb.c: cmd_ruby()
         ↓
ruby_eval(sh, "puts 'Hello'")
         ↓
Allocate compiler context (mrc_ccontext)
         ↓
Parse source → AST (Prism)
         ↓
Compile AST → IR (mruby-compiler2)
         ↓
Dump IR → Bytecode
         ↓
Free compiler context
         ↓
Create mrubyc VM task
         ↓
Execute bytecode
         ↓
Output: Hello
```

### File Loading Execution

```
User: ruby_file /lfs/script.rb
         ↓
shell_irb.c: cmd_ruby_file()
         ↓
Open file with Zephyr FS API
         ↓
Allocate 16KB buffer (k_malloc)
         ↓
Read file contents
         ↓
Close file
         ↓
ruby_eval(sh, file_contents)
         ↓
[Same as direct execution]
         ↓
Free file buffer
```

## Configuration Options

### Three Main Modes

#### 1. VM Only Mode (Minimal)
```ini
CONFIG_HAKO=y
# No compiler options
```
- Use pre-compiled bytecode only
- Smallest footprint (~40 KB ROM, 40 KB RAM)
- Best for production with fixed Ruby code

#### 2. VM + Compiler Mode (Development)
```ini
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
```
- Full on-device compilation
- Interactive development
- ~500-700 KB ROM, ~105 KB peak RAM

#### 3. VM + Compiler + Filesystem Mode (Full)
```ini
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_HAKO_FILE_SUPPORT=y
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
```
- Everything including file loading
- Persistent Ruby scripts
- ~500-700 KB ROM, ~121 KB peak RAM

## Build System Integration

### Kconfig Integration

The module integrates with Zephyr's Kconfig system:

```
zephyr/Kconfig.zephyr
    ↓
source "modules/lib/hako/Kconfig"
    ↓
HAKO (main toggle)
    ├── HAKO_COMPILER (compiler subsystem)
    │   ├── HAKO_COMPILER_OPTIMIZE_SIZE
    │   ├── HAKO_COMPILER_MEMORY_SIZE
    │   └── HAKO_EVAL
    │       └── HAKO_IRB_COMMAND
    │           └── HAKO_FILE_SUPPORT
    └── Memory/VM configuration
```

### CMake Integration

```
zephyr/CMakeLists.txt
    ↓
modules/lib/hako/CMakeLists.txt
    ↓
if(CONFIG_HAKO)
    zephyr_library()
    zephyr_library_sources(ext/hako/src/*.c)

    if(CONFIG_HAKO_COMPILER)
        zephyr_library_sources(
            ext/picoruby/mruby-compiler2/src/*.c
            ext/picoruby/mruby-compiler2/lib/prism/src/*.c
        )
    endif()

    if(CONFIG_HAKO_IRB_COMMAND)
        zephyr_library_sources(zephyr/shell_irb.c)
    endif()
endif()
```

## Shell Command Integration

The module registers shell commands when `CONFIG_SHELL=y`:

```c
SHELL_CMD_REGISTER(ruby, NULL, "Execute Ruby expression", cmd_ruby);
SHELL_CMD_REGISTER(ruby_info, NULL, "Show Ruby info", cmd_ruby_info);
SHELL_CMD_REGISTER(ruby_file, NULL, "Load Ruby file", cmd_ruby_file);
```

These commands are available in all Zephyr shells (UART, USB, RTT, etc.).

## Filesystem Integration

When `CONFIG_HAKO_FILE_SUPPORT=y`:

```
Application
    ↓
ruby_file /lfs/script.rb
    ↓
Zephyr FS API (fs_open, fs_read, fs_close)
    ↓
LittleFS / FAT / NVS / etc.
    ↓
Flash / SD Card / RAM
```

Supports any Zephyr filesystem backend:
- LittleFS (flash-based, wear-leveling)
- FAT (SD cards, USB)
- NVS (key-value storage)
- Custom filesystems

## Thread Safety

### Current Implementation (Single-threaded)
- mrubyc VM is **NOT** thread-safe
- Compiler is **NOT** thread-safe
- Shell commands execute in shell thread context
- No concurrent Ruby execution

### Recommendations
- Use mutex if accessing VM from multiple threads
- Keep Ruby execution in single thread
- Use Zephyr message queues to communicate with Ruby

## Performance Characteristics

### Compilation Performance
- Simple expression: ~10-50 ms
- Small script (10 lines): ~50-200 ms
- Medium script (100 lines): ~200-500 ms
- Depends on: CPU speed, memory speed, script complexity

### Execution Performance
- Similar to interpreted Python
- ~10-100x slower than native C
- Deterministic (no JIT warmup)
- Suitable for: Configuration, scripting, automation
- Not suitable for: Real-time control loops, DSP, high-frequency tasks

## Limitations

### Language Features
- No threads (use Zephyr threads from C)
- No native extensions (must be written in C and registered)
- Limited standard library (embedded-focused)
- No require/load (all code must be explicit)

### Memory Constraints
- VM pool size is fixed at initialization
- No dynamic pool growth
- Out-of-memory = garbage collection or error

### Compilation Constraints
- Max source file size: 16 KB default
- Compilation requires temporary memory spike
- Cannot compile if heap is fragmented

## Future Enhancements

### Potential Improvements
1. **require/load support** - Load multiple Ruby files with dependencies
2. **Pre-compilation tool** - Compile on host, embed bytecode
3. **Native extension API** - Register C functions as Ruby methods
4. **Thread-safe VM** - Support concurrent Ruby tasks
5. **Debugging support** - Breakpoints, step execution
6. **Module system** - Package management for Ruby libraries

## References

### Component Projects
- [mrubyc](https://github.com/mrubyc/mrubyc) - Lightweight Ruby VM
- [PicoRuby](https://github.com/picoruby/picoruby) - Ruby for embedded systems
- [mruby-compiler2](https://github.com/picoruby/mruby-compiler2) - PicoRuby compiler
- [Prism](https://github.com/ruby/prism) - Universal Ruby parser

### Related Documentation
- [CONFIGURATION.md](CONFIGURATION.md) - Detailed Kconfig options
- [FILE_SUPPORT.md](../FILE_SUPPORT.md) - Using filesystem with Ruby
- [API.md](API.md) - C API reference (if exposing programmatic API)

## License

This module integrates components with various licenses:
- mrubyc: BSD-3-Clause
- PicoRuby: MIT
- Prism: MIT
- Zephyr integration: BSD-3-Clause

See individual component LICENSE files for details.
