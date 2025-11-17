# Hako - Ruby for IoT and Embedded Systems

A Zephyr RTOS module that brings Ruby programming to embedded systems using the Mruby/c VM, PicoRuby compiler, and an extensible package system.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Zephyr](https://img.shields.io/badge/Zephyr-RTOS-purple.svg)](https://www.zephyrproject.org/)
[![Ruby](https://img.shields.io/badge/Ruby-Compatible-red.svg)](https://www.ruby-lang.org/)

## What is Hako?

Hako (箱, meaning "box" in Japanese) is a complete Ruby runtime for embedded systems and IoT devices. It enables you to write, compile, and execute Ruby code directly on microcontrollers running Zephyr RTOS.

### Key Features

- **Lightweight Mruby/c VM** - Ultra-compact bytecode interpreter (~40 KB ROM, 40 KB RAM)
- **On-Device Compilation** - PicoRuby compiler with Prism parser (optional, +500 KB ROM)
- **Interactive Shell** - REPL-like experience via Zephyr shell
- **Filesystem Support** - Load Ruby scripts from LittleFS, FAT, or other filesystems
- **Extensible** - Create C extensions with Ruby bindings
- **Package System** - Built-in require/load support for modular code
- **Highly Configurable** - Scale from minimal VM (40KB) to full development environment (500KB)
- **Zero External Dependencies** - Uses only Zephyr standard libraries

## Table of Contents

- [Quick Start](#quick-start)
- [Complete Getting Started Tutorial](#complete-getting-started-tutorial)
- [Architecture](#architecture)
- [Ruby Compilation Workflow](#ruby-compilation-workflow)
  - [Runtime Compilation (On-Device)](#runtime-compilation-on-device)
  - [Pre-Compilation (Build-Time)](#pre-compilation-build-time)
- [Configuration Modes](#configuration-modes)
- [Shell Commands](#shell-commands)
- [Memory Usage](#memory-usage)
- [Filesystem Integration](#filesystem-integration)
- [Configuration Reference](#configuration-reference)
- [Supported Ruby Features](#supported-ruby-features)
- [Example Application](#example-application)
- [Hako Loader API Reference](#hako-loader-api-reference)
- [Creating Hako Extensions](#creating-hako-extensions)
- [Thread Safety](#thread-safety)
- [Requirements](#requirements)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)
- [Credits and Acknowledgments](#credits-and-acknowledgments)
- [Roadmap](#roadmap)

## Quick Start

### 1. Add to Your Project

The module should be located at `modules/lib/hako/` in your Zephyr workspace.

### 2. Enable in Configuration

**Minimal (VM only, pre-compiled bytecode)**:
```ini
# prj.conf
CONFIG_HAKO=y
```

**Full Development (interactive Ruby with shell)**:
```ini
# prj.conf
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_SHELL=y
CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
```

### 3. Build and Run

```bash
west build -b <your_board>
west flash

# Or for QEMU testing
west build -b qemu_x86
west build -t run
```

### 4. Use Ruby Shell

```
uart:~$ ruby "puts 'Hello from Ruby!'"
Hello from Ruby!

uart:~$ ruby "3.times { |i| puts i }"
0
1
2

uart:~$ ruby_info
PicoRuby Compiler for Hako
  Compiler: mruby-compiler2
  Parser: Prism (Ruby universal parser)
  VM: Mruby/c
```

---

## Complete Getting Started Tutorial

### Prerequisites

Before you start, ensure you have:
- **Zephyr SDK 0.16.0+** installed
- **West build tool** configured
- **CMake 3.20+**
- A supported development board or QEMU

If you don't have Zephyr set up yet, follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

### Step 1: Create a New Zephyr Application

```bash
# Create application directory
mkdir my-ruby-app
cd my-ruby-app

# Initialize as Zephyr application
west init -l .
west update
```

### Step 2: Add Hako Module

Create or edit `west.yml`:

```yaml
manifest:
  projects:
    - name: hako
      url: https://github.com/your-org/hako
      revision: main
      path: modules/lib/hako
```

Then update:
```bash
west update
```

### Step 3: Configure Your Application

Create `prj.conf` with your desired configuration:

**For Development (Interactive Ruby):**
```ini
# Enable Hako with compiler
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y

# Enable Zephyr shell
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_SERIAL=y

# Memory configuration
CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096

# Optional: Logging
CONFIG_LOG=y
CONFIG_HAKO_LOG_LEVEL=3
```

**For Production (Pre-compiled Bytecode):**
```ini
# Minimal configuration
CONFIG_HAKO=y

# Smaller memory footprint
CONFIG_HEAP_MEM_POOL_SIZE=65536
CONFIG_MAIN_STACK_SIZE=2048
```

### Step 4: Write Your Application

Create `src/main.c`:

```c
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <mrubyc.h>

#define MEMORY_SIZE (1024 * 40)
static uint8_t memory_pool[MEMORY_SIZE];

void main(void)
{
    printk("Hako Ruby Runtime Starting...\n");

    // Initialize Mruby/c VM
    mrbc_init_alloc(memory_pool, MEMORY_SIZE);
    mrbc_init_global();

    printk("Mruby/c VM initialized\n");
    printk("Type 'ruby \"puts 'Hello'\"' to execute Ruby code\n");

    // Main loop (shell runs in background)
    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
```

### Step 5: Build and Flash

```bash
# For QEMU (testing)
west build -b qemu_x86
west build -t run

# For real hardware (example: STM32)
west build -b stm32f4_disco
west flash

# Connect to serial console
minicom -D /dev/ttyUSB0 -b 115200
```

### Step 6: Test Ruby Code

In the serial console:

```
*** Booting Zephyr OS build v3.5.0 ***
Hako Ruby Runtime Starting...
Mruby/c VM initialized
Type 'ruby "puts 'Hello'"' to execute Ruby code

uart:~$ ruby "puts 'Hello from Mruby/c!'"
Hello from Mruby/c!

uart:~$ ruby "5.times { |i| puts 'Count: ' + i.to_s }"
Count: 0
Count: 1
Count: 2
Count: 3
Count: 4

uart:~$ ruby "x = [1,2,3]; x.map { |n| n * 2 }"
[2, 4, 6]
```

### Step 7: Add Pre-compiled Ruby Scripts

Create `src/ruby/main.rb`:

```ruby
# This will be compiled to bytecode at build time
puts "Hello from embedded Ruby!"
puts "HAKO is running on Zephyr RTOS"

result = 42 + 8
puts "The answer is: #{result}"

numbers = [1, 2, 3, 4, 5]
sum = 0
numbers.each do |n|
  sum += n
end
puts "Sum of #{numbers}: #{sum}"

puts "Ruby application completed!"
```

Update `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_ruby_app)

# Add C sources
target_sources(app PRIVATE src/main.c)

# Automatically compile all Ruby files in src/ruby/ and lib/
# This creates a registry with all bytecode
hako_auto_add_ruby()
```

Update `src/main.c` to use the Hako loader:

```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hako/loader.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Auto-generated bytecode registry (created by hako_auto_add_ruby) */
#include "my_ruby_app_registry.h"

int main(void)
{
    int ret;

    LOG_INF("=== My Ruby Application ===");
    LOG_INF("Initializing Mruby/c VM...");

    /* Initialize Hako VM (handles memory pool automatically) */
    ret = hako_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize VM: %d", ret);
        return ret;
    }

    /* Load all compiled bytecode from registry */
    LOG_INF("Loading Ruby bytecode...");
    ret = hako_load_registry(hako_my_ruby_app_registry,
                             hako_my_ruby_app_registry_count);
    if (ret < 0) {
        LOG_ERR("Failed to load bytecode: %d", ret);
        return ret;
    }

    /* Find and load the main script */
    const uint8_t *main_bytecode = hako_find_bytecode("main");
    if (!main_bytecode) {
        LOG_ERR("main.rb not found in registry!");
        return -ENOENT;
    }

    ret = hako_load_bytecode("main", main_bytecode);
    if (ret < 0) {
        LOG_ERR("Failed to load main: %d", ret);
        return ret;
    }

    /* Run the Ruby VM */
    LOG_INF("Starting Ruby application...");
    ret = hako_run();
    if (ret < 0) {
        LOG_ERR("Ruby execution failed: %d", ret);
        return ret;
    }

    LOG_INF("Ruby application completed successfully");
    return 0;
}
```

Enable logging in `prj.conf`:
```ini
CONFIG_HAKO=y
CONFIG_LOG=y
CONFIG_HAKO_LOG_LEVEL=3
CONFIG_HEAP_MEM_POOL_SIZE=65536
CONFIG_MAIN_STACK_SIZE=2048
```

Rebuild and flash:
```bash
west build -b qemu_x86
west flash

# Expected output:
# === My Ruby Application ===
# Initializing Mruby/c VM...
# Loading Ruby bytecode...
# Starting Ruby application...
# Hello from embedded Ruby!
# HAKO is running on Zephyr RTOS
# The answer is: 50
# Sum of [1, 2, 3, 4, 5]: 15
# Ruby application completed!
```

**Key Points:**
- `hako_auto_add_ruby()` automatically finds all `.rb` files in `src/ruby/` and `lib/`
- Creates a registry header: `<project_name>_registry.h`
- Registry naming: `hako_<project_name>_registry` and `hako_<project_name>_registry_count`
- Use `hako_init()` instead of manual `mrbc_init_alloc()`
- Use `hako_run()` instead of raw `mrbc_run()`

### Step 8: Add GPIO Extension (Optional)

Enable GPIO extension in `prj.conf`:
```ini
CONFIG_HAKO_ZEPHYR_GPIO=y
CONFIG_GPIO=y
```

Add LED devicetree alias in your board overlay (`boards/your_board.overlay`):
```dts
/ {
    aliases {
        led0 = &led_0;
    };
};
```

Use in Ruby:
```ruby
led = Zephyr::GPIO.open(:led0, mode: :output)
led.write(1)  # Turn on
sleep 1
led.write(0)  # Turn off
led.toggle    # Toggle state
```
---

## Architecture

Hako is built as a layered system that integrates seamlessly with Zephyr RTOS:

```
┌─────────────────────────────────────────────────────┐
│          User Application / Shell                   │
│  (Your Ruby code, interactive commands)             │
├─────────────────────────────────────────────────────┤
│   Shell Commands                                    │
│   • ruby <expression>    - Execute Ruby code        │
│   • ruby_file <path>     - Load Ruby script         │
│   • ruby_info            - Show VM information      │
├─────────────────────────────────────────────────────┤
│   PicoRuby Compiler (Optional, ~500 KB ROM)         │
│   ├─ mruby-compiler2 - Ruby to bytecode compiler    │
│   ├─ Prism Parser - Modern Ruby parser              │
│   └─ eval() support - Runtime code compilation      │
├─────────────────────────────────────────────────────┤
│   Mruby/c VM (~40 KB ROM, 40 KB RAM)                │
│   ├─ Bytecode Interpreter - Execute Ruby bytecode   │
│   ├─ Core Classes - String, Array, Hash, etc.       │
│   └─ Memory Management - Allocation & GC            │
├─────────────────────────────────────────────────────┤
│   Hako Extensions (Optional, modular)               │
│   ├─ Zephyr::GPIO - GPIO control from Ruby          │
│   └─ Custom Extensions - Your hardware bindings     │
├─────────────────────────────────────────────────────┤
│   Zephyr RTOS Integration Layer                     │
│   ├─ HAL - Memory, console I/O, timing              │
│   ├─ Shell Integration - Command registration       │
│   ├─ Filesystem - LittleFS, FAT support             │
│   └─ POSIX API - Standard C library functions       │
└─────────────────────────────────────────────────────┘
          ↓
┌─────────────────────────────────────────────────────┐
│              Zephyr RTOS Kernel                     │
│   Threads • Memory • Drivers • Networking           │
└─────────────────────────────────────────────────────┘
```

### How It Works

1. **Ruby Source Code** → Written by you in `.rb` files or shell commands
2. **Compilation** → PicoRuby compiler converts Ruby to mruby bytecode
3. **Execution** → Mruby/c VM interprets and executes the bytecode
4. **Integration** → Zephyr HAL provides system services (memory, I/O, timers)
5. **Extensions** → C functions exposed to Ruby for hardware access

## Ruby Compilation Workflow

Hako supports two compilation workflows: **on-device** (runtime) and **pre-compilation** (build-time).

### Runtime Compilation (On-Device)

When you execute Ruby code via the shell, this happens:

```
┌─────────────────────────────────────────────────────────┐
│  uart:~$ ruby "puts 'Hello!'"                           │
└──────────────────┬──────────────────────────────────────┘
                   ↓
         ┌─────────────────────┐
         │  Shell Command      │
         │  Handler            │
         └──────┬──────────────┘
                ↓
    ┌───────────────────────────┐
    │  1. Allocate Compiler     │
    │     Memory Pool           │
    │     (~65 KB)              │
    └──────────┬────────────────┘
               ↓
    ┌───────────────────────────┐
    │  2. Prism Parser          │
    │     Parse Ruby source     │
    │     → AST (Abstract       │
    │        Syntax Tree)       │
    └──────────┬────────────────┘
               ↓
    ┌───────────────────────────┐
    │  3. mruby-compiler2       │
    │     AST → mruby bytecode  │
    │     (optimized opcodes)   │
    └──────────┬────────────────┘
               ↓
    ┌───────────────────────────┐
    │  4. Mruby/c VM            │
    │     Execute bytecode      │
    │     in VM context         │
    └──────────┬────────────────┘
               ↓
    ┌───────────────────────────┐
    │  5. Free Compiler         │
    │     Memory                │
    │     (Keep VM memory)      │
    └──────────┬────────────────┘
               ↓
         ┌──────────────┐
         │  Output:     │
         │  Hello!      │
         └──────────────┘
```

**Memory Usage During Runtime Compilation:**
- VM Pool: 40 KB (always allocated)
- Compiler Pool: 65 KB (allocated during compilation, freed after)
- Peak: ~105 KB total

### Pre-Compilation (Build-Time)

For production deployments, compile Ruby to C arrays at build time:

```
┌─────────────────────────────────────────────────────────┐
│  Development Machine (x86/x64)                          │
├─────────────────────────────────────────────────────────┤
│                                                          │
│   my_script.rb                                          │
│   ┌──────────────────────┐                              │
│   │ puts "Hello!"        │                              │
│   │ 5.times { |i|       │                              │
│   │   puts "Count: #{i}" │                              │
│   │ }                    │                              │
│   └──────────┬───────────┘                              │
│              ↓                                           │
│   ┌──────────────────────────────────┐                  │
│   │  mrbc Compiler (mruby bytecode)  │                  │
│   │  (Host tool, not on device)      │                  │
│   └──────────┬───────────────────────┘                  │
│              ↓                                           │
│   ┌──────────────────────────────────┐                  │
│   │  my_script.c (Generated)         │                  │
│   │  const uint8_t bytecode[] = {    │                  │
│   │    0x52, 0x49, 0x54, 0x45, ...   │                  │
│   │  };                              │                  │
│   └──────────┬───────────────────────┘                  │
│              ↓                                           │
│   ┌──────────────────────────────────┐                  │
│   │  CMake Integration               │                  │
│   │  hako_compile_ruby_to_c()        │                  │
│   └──────────┬───────────────────────┘                  │
│              ↓                                           │
│   ┌──────────────────────────────────┐                  │
│   │  Link into firmware              │                  │
│   │  (Embedded in Flash)             │                  │
│   └──────────┬───────────────────────┘                  │
└──────────────┼──────────────────────────────────────────┘
               ↓
┌──────────────────────────────────────────────────────────┐
│  Target Device (ARM Cortex-M, RISC-V, etc.)              │
├──────────────────────────────────────────────────────────┤
│                                                           │
│   Flash Memory                                           │
│   ┌──────────────────────────────────┐                   │
│   │  Firmware                        │                   │
│   │  ├─ Zephyr kernel                │                   │
│   │  ├─ Mruby/c VM (~40 KB)          │                   │
│   │  └─ bytecode[] array             │                   │
│   └──────────┬───────────────────────┘                   │
│              ↓                                            │
│   ┌──────────────────────────────────┐                   │
│   │  C Code                          │                   │
│   │  mrbc_load_mrb(bytecode);        │                   │
│   │  mrbc_run();                     │                   │
│   └──────────┬───────────────────────┘                   │
│              ↓                                            │
│   ┌──────────────────────────────────┐                   │
│   │  Mruby/c VM executes bytecode    │                   │
│   │  (No compiler needed!)           │                   │
│   └──────────────────────────────────┘                   │
└──────────────────────────────────────────────────────────┘
```

**Advantages of Pre-Compilation:**
- **Smaller ROM**: No compiler needed (~460 KB saved)
- **Faster Startup**: No compilation delay
- **Lower RAM**: No compiler memory pool needed (~65 KB saved)
- **Production Ready**: Code is fixed and tested

**CMake Integration - Method 1: Automatic (Recommended)**

```cmake
# In your application CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)

target_sources(app PRIVATE src/main.c)

# Automatically compile all .rb files in src/ruby/ and lib/
# Creates registry header: my_app_registry.h
hako_auto_add_ruby()
```

**CMake Integration - Method 2: Manual Control**

```cmake
# In your application CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)

# Compile multiple Ruby files into a library
hako_add_ruby_library(
    NAME my_scripts
    SOURCES
        src/ruby/init.rb
        src/ruby/config.rb
        src/ruby/app.rb
    TARGET app
)

target_sources(app PRIVATE src/main.c)

# Then in C code:
# #include "my_scripts_registry.h"
# hako_load_registry(hako_my_scripts_registry, hako_my_scripts_registry_count);
```

**Using in C Code:**

```c
#include <hako/loader.h>
#include "my_app_registry.h"  /* Auto-generated */

int main(void) {
    /* Initialize VM */
    hako_init();

    /* Load all bytecode */
    hako_load_registry(hako_my_app_registry,
                       hako_my_app_registry_count);

    /* Load and run main script */
    const uint8_t *bytecode = hako_find_bytecode("main");
    hako_load_bytecode("main", bytecode);
    hako_run();

    return 0;
}
```

See [cmake/hako.cmake](cmake/hako.cmake) for all CMake utilities.

## Configuration Modes

### 1. VM Only (Minimal)
**ROM**: ~40 KB | **RAM**: ~40 KB

```ini
CONFIG_HAKO=y
```

- Execute pre-compiled mruby bytecode
- Smallest footprint
- Best for production with fixed scripts

### 2. VM + Compiler (Development)
**ROM**: ~500 KB | **RAM**: ~105 KB peak

```ini
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_SHELL=y
CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
```

- Compile Ruby source at runtime
- Interactive REPL-like experience
- Development and testing

### 3. VM + Compiler + Filesystem (Full)
**ROM**: ~500 KB | **RAM**: ~121 KB peak

```ini
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_HAKO_FILE_SUPPORT=y

CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y

CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
```

- Load Ruby scripts from files
- Persistent storage
- Application scripting

## Shell Commands

When `CONFIG_HAKO_IRB_COMMAND=y`:

### `ruby <expression>`
Execute a Ruby expression:
```bash
uart:~$ ruby "puts 'Hello!'"
uart:~$ ruby "x = 10; puts x * 2"
uart:~$ ruby "arr = [1,2,3]; arr.each { |n| puts n }"
```

### `ruby_file <path>`
Load and execute Ruby script from file (requires `CONFIG_HAKO_FILE_SUPPORT=y`):
```bash
uart:~$ ruby_file /lfs/script.rb
uart:~$ ruby_file /sd/app.rb
```

### `ruby_info`
Display compiler and VM information:
```bash
uart:~$ ruby_info
PicoRuby Compiler for Hako
  Compiler: mruby-compiler2
  Parser: Prism (Ruby universal parser)
  VM: Mruby/c
```

## Memory Usage

Understanding memory requirements helps you choose the right configuration for your hardware.

### ROM (Flash) Requirements

| Component | Size | Configuration |
|-----------|------|---------------|
| **Mruby/c VM** | ~40 KB | `CONFIG_HAKO=y` |
| PicoRuby compiler | ~250 KB | `CONFIG_HAKO_COMPILER=y` |
| Prism parser (optimized) | ~200 KB | `CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y` |
| Prism parser (full) | ~400 KB | Optimization disabled |
| **Minimum (VM only)** | **~40 KB** | Production with pre-compiled bytecode |
| **Recommended (VM + Compiler)** | **~500 KB** | Development with on-device compilation |
| **Maximum (VM + Full Compiler)** | **~700 KB** | Advanced development features |

### RAM Requirements

| Pool | Size | When Allocated | Configuration |
|------|------|----------------|---------------|
| **VM pool** | 40 KB (default) | Always | `CONFIG_HAKO_MEMORY_SIZE` |
| Compiler pool | 65 KB (default) | During compilation only | `CONFIG_HAKO_COMPILER_MEMORY_SIZE` |
| File buffer | 16 KB | During file load only | `CONFIG_HAKO_FILE_SUPPORT=y` |
| Stack (recommended) | 4 KB+ | Always | `CONFIG_MAIN_STACK_SIZE` |
| Heap (recommended) | 128 KB+ | Always | `CONFIG_HEAP_MEM_POOL_SIZE` |
| **Peak (VM only)** | **~40 KB** | VM runtime execution | |
| **Peak (VM + Compilation)** | **~105 KB** | Compiling and executing | |
| **Peak (VM + File Load)** | **~121 KB** | Loading large scripts | |

**Memory Optimization Tips:**
- Use pre-compilation for production to eliminate compiler memory
- Adjust `CONFIG_HAKO_MEMORY_SIZE` based on script complexity
- Free compiler pool after compilation (automatic in shell commands)
- Use `CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y` to reduce ROM by 30-40%

## Filesystem Integration

Supports any Zephyr filesystem:

### LittleFS (Recommended for Flash)
```ini
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FS_LITTLEFS_FMP_DEV=y
```

### FAT (For SD Cards)
```ini
CONFIG_FILE_SYSTEM=y
CONFIG_FAT_FILESYSTEM_ELM=y
CONFIG_DISK_ACCESS=y
```

### Example: Setup Filesystem in C
```c
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

static struct fs_mount_t mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = "/lfs",
};

void main(void) {
    fs_mount(&mnt);

    // Now you can use: ruby_file /lfs/script.rb
}
```

## Example Ruby Scripts

### Hello World
```ruby
puts 'Hello from Ruby!'
5.times do |i|
  puts "Count: #{i}"
end
```

### Variables and Math
```ruby
x = 10
y = 20
puts "x + y = #{x + y}"
puts "x * y = #{x * y}"
```

### Arrays and Iteration
```ruby
arr = [1, 2, 3, 4, 5]
arr.each { |n| puts "Item: #{n}" }
```

## Configuration Reference

Hako provides extensive configuration options through Zephyr's Kconfig system.

### Core VM Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CONFIG_HAKO` | bool | n | Enable Hako Ruby runtime with Mruby/c VM |
| `CONFIG_HAKO_MEMORY_SIZE` | int | 32768 | VM memory pool size in bytes (adjust based on script complexity) |
| `CONFIG_HAKO_LOG_LEVEL` | int | 3 | Log level: 0=OFF, 1=ERR, 2=WRN, 3=INF, 4=DBG |
| `CONFIG_HAKO_TICK_UNIT` | int | 10 | Scheduler tick unit in milliseconds (1-100) |
| `CONFIG_HAKO_TIMESLICE_TICK_COUNT` | int | 1 | Number of ticks per task timeslice |
| `CONFIG_HAKO_USE_MATH` | bool | y | Enable Math module (sin, cos, sqrt, etc.) |
| `CONFIG_HAKO_CONVERT_CRLF` | bool | n | Convert LF to CRLF in console output |

### Compiler Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CONFIG_HAKO_COMPILER` | bool | n | Enable PicoRuby compiler (mruby-compiler2 + Prism) |
| `CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE` | bool | y | Optimize compiler for size (saves ~200 KB ROM) |
| `CONFIG_HAKO_COMPILER_MEMORY_SIZE` | int | 65536 | Compiler memory pool size in bytes |
| `CONFIG_HAKO_COMPILER_DEBUG` | bool | n | Enable compiler debug output (large overhead) |
| `CONFIG_HAKO_EVAL` | bool | y | Enable Kernel.eval() for runtime code execution |
| `CONFIG_HAKO_IRB_COMMAND` | bool | y | Register shell commands (ruby, ruby_file, ruby_info) |
| `CONFIG_HAKO_FILE_SUPPORT` | bool | y | Enable loading Ruby scripts from filesystem |

### Standard Library Gems

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CONFIG_HAKO_REQUIRE` | bool | y | Enable require/load support (picoruby-require) |
| `CONFIG_HAKO_SANDBOX` | bool | y | Enable sandbox execution (picoruby-sandbox) |
| `CONFIG_HAKO_METAPROG` | bool | n | Enable metaprogramming (send, methods, respond_to?) |

### Extensions

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CONFIG_HAKO_ZEPHYR_GPIO` | bool | n | Enable Zephyr::GPIO Ruby API for GPIO control |

### Recommended Configurations

**1. Minimal Production (40 KB ROM, 40 KB RAM)**
```ini
# VM only, pre-compiled bytecode
CONFIG_HAKO=y
CONFIG_HAKO_MEMORY_SIZE=32768
CONFIG_HEAP_MEM_POOL_SIZE=65536
CONFIG_MAIN_STACK_SIZE=2048
```

**2. Development with Shell (500 KB ROM, 105 KB RAM)**
```ini
# Full interactive development
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_HAKO_REQUIRE=y
CONFIG_HAKO_SANDBOX=y

CONFIG_SHELL=y
CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
```

**3. Production with Filesystem (60 KB ROM, 56 KB RAM)**
```ini
# VM + filesystem support, no compiler
CONFIG_HAKO=y
CONFIG_HAKO_MEMORY_SIZE=40960

CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y

CONFIG_HEAP_MEM_POOL_SIZE=81920
CONFIG_MAIN_STACK_SIZE=3072
```

**4. Maximum Features (700 KB ROM, 121 KB RAM)**
```ini
# Everything enabled
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_HAKO_FILE_SUPPORT=y
CONFIG_HAKO_REQUIRE=y
CONFIG_HAKO_SANDBOX=y
CONFIG_HAKO_METAPROG=y
CONFIG_HAKO_ZEPHYR_GPIO=y

# Disable size optimization for full features
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=n

CONFIG_SHELL=y
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_GPIO=y

CONFIG_HEAP_MEM_POOL_SIZE=196608
CONFIG_MAIN_STACK_SIZE=8192
```

### Memory Tuning Guidelines

**VM Memory (`CONFIG_HAKO_MEMORY_SIZE`)**:
- Simple scripts: 8192-16384 bytes
- Moderate scripts with data structures: 16384-32768 bytes
- Complex applications: 32768-65536 bytes
- Large data processing: 65536+ bytes

**Compiler Memory (`CONFIG_HAKO_COMPILER_MEMORY_SIZE`)**:
- Small scripts (<50 lines): 32768 bytes
- Medium scripts (50-200 lines): 65536 bytes (default)
- Large scripts (200+ lines): 131072 bytes

**System Heap (`CONFIG_HEAP_MEM_POOL_SIZE`)**:
- VM only: 65536 bytes
- VM + Compiler: 131072 bytes
- VM + Compiler + Filesystem: 196608 bytes

**Stack Size (`CONFIG_MAIN_STACK_SIZE`)**:
- Minimal: 2048 bytes
- Recommended: 4096 bytes
- With deep recursion: 8192+ bytes

### Configuration Dependencies

Some options automatically enable dependencies:

```
CONFIG_HAKO
  └─ Enables: CONFIG_POSIX_API (automatic)

CONFIG_HAKO_COMPILER
  └─ Requires: CONFIG_SHELL

CONFIG_HAKO_IRB_COMMAND
  └─ Requires: CONFIG_SHELL

CONFIG_HAKO_FILE_SUPPORT
  ├─ Requires: CONFIG_HAKO_IRB_COMMAND
  └─ Requires: CONFIG_FILE_SYSTEM

CONFIG_HAKO_REQUIRE
  ├─ Requires: CONFIG_HAKO_EVAL
  └─ Requires: CONFIG_FILE_SYSTEM

CONFIG_HAKO_SANDBOX
  └─ Requires: CONFIG_HAKO_COMPILER

CONFIG_HAKO_ZEPHYR_GPIO
  └─ Requires: CONFIG_GPIO
```

See [Kconfig](Kconfig) for complete configuration options.

## Supported Ruby Features

### Core Classes
- Integer, Float, String, Symbol
- Array, Hash, Range
- Proc, Method
- Class, Module
- Exception

### Language Features
- Variables (local, instance, class, global)
- Methods and blocks
- Classes and inheritance
- Modules and mixins
- Iterators (each, map, select, etc.)
- String interpolation
- Exception handling (begin/rescue/ensure)

### Limitations

Understanding what Hako doesn't support helps you design better applications:

- **No Ruby Threads**: Use Zephyr threads and message queues instead
- **Limited Standard Library**: Core classes only (no File I/O, Socket, etc. in Ruby - use C extensions)
- **No JIT Compilation**: Bytecode is interpreted (slower than native C)
- **No require/load by default**: Enable `CONFIG_HAKO_REQUIRE` or use explicit loading
- **No C Extensions from Ruby**: Must write C extensions

### When to Use Hako

**✅ Great For:**
- Configuration and settings management
- Scripting hardware behavior
- Rapid prototyping on embedded devices
- Domain-specific logic (business rules, state machines)
- Interactive debugging via shell
- Hot-reloading application logic from filesystem
- Teaching embedded programming with Ruby

## Hako Loader API Reference

The Hako loader provides a high-level API for managing Ruby bytecode in your application.

### Core Functions

#### `int hako_init(void)`

Initializes the Mruby/c VM and allocates resources.

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
int ret = hako_init();
if (ret < 0) {
    LOG_ERR("VM initialization failed: %d", ret);
    return ret;
}
```

**Details:**
- Allocates VM memory pool (size from `CONFIG_HAKO_MEMORY_SIZE`)
- Initializes global state
- Registers built-in classes
- Must be called before any other Hako functions

---

#### `int hako_load_registry(const struct hako_bytecode_entry *registry, size_t count)`

Loads all bytecode modules from a registry created by CMake.

**Parameters:**
- `registry` - Pointer to bytecode registry array (NULL-terminated)
- `count` - Number of entries (excluding NULL terminator)

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
#include "my_app_registry.h"  // Auto-generated by hako_auto_add_ruby()

ret = hako_load_registry(hako_my_app_registry,
                         hako_my_app_registry_count);
if (ret < 0) {
    LOG_ERR("Failed to load registry: %d", ret);
}
```

**Details:**
- Registers all bytecode for `require()` to find
- Created automatically by `hako_auto_add_ruby()` or `hako_add_ruby_library()`
- Registry name: `hako_<project_name>_registry`
- Count variable: `hako_<project_name>_registry_count`

---

#### `const uint8_t *hako_find_bytecode(const char *name)`

Finds bytecode in the loaded registry by module name.

**Parameters:**
- `name` - Module name (without `.rb` extension)

**Returns:** Pointer to bytecode if found, NULL otherwise

**Example:**
```c
const uint8_t *main_bc = hako_find_bytecode("main");
if (!main_bc) {
    LOG_ERR("main.rb not found!");
    return -ENOENT;
}
```

**Details:**
- Searches currently loaded registry
- Case-sensitive name matching
- Returns raw bytecode pointer (not loaded into VM yet)

---

#### `int hako_load_bytecode(const char *name, const uint8_t *bytecode)`

Loads a single bytecode module into the VM.

**Parameters:**
- `name` - Module name (for debugging/logging)
- `bytecode` - Pointer to mruby bytecode array

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
const uint8_t *main_bc = hako_find_bytecode("main");
ret = hako_load_bytecode("main", main_bc);
if (ret < 0) {
    LOG_ERR("Failed to load main: %d", ret);
}
```

**Details:**
- Parses and validates bytecode
- Creates VM context for execution
- Can be called multiple times for different modules

---

#### `int hako_run(void)`

Executes all loaded Ruby bytecode.

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
ret = hako_run();
if (ret < 0) {
    LOG_ERR("Ruby execution failed: %d", ret);
}
```

**Details:**
- Blocks until Ruby code completes
- Handles exceptions and errors
- Can be called only once per VM initialization
- VM state is preserved after execution

---

### Data Structures

#### `struct hako_bytecode_entry`

Represents a single Ruby bytecode module in the registry.

```c
struct hako_bytecode_entry {
    const char *name;           /* Module name (without .rb) */
    const uint8_t *bytecode;    /* Pointer to bytecode array */
};
```

**Example Registry (auto-generated):**
```c
const struct hako_bytecode_entry hako_my_app_registry[] = {
    {"main", mrb_my_app_main},
    {"config", mrb_my_app_config},
    {"utils", mrb_my_app_utils},
    {NULL, NULL}  // Terminator
};

const size_t hako_my_app_registry_count = 3;
```

---

### Complete Example

```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hako/loader.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Auto-generated by hako_auto_add_ruby() */
#include "my_app_registry.h"

int main(void)
{
    int ret;

    /* 1. Initialize VM */
    LOG_INF("Initializing Mruby/c VM...");
    ret = hako_init();
    if (ret < 0) {
        LOG_ERR("hako_init failed: %d", ret);
        return ret;
    }

    /* 2. Load registry (all bytecode modules) */
    LOG_INF("Loading bytecode registry...");
    ret = hako_load_registry(hako_my_app_registry,
                             hako_my_app_registry_count);
    if (ret < 0) {
        LOG_ERR("hako_load_registry failed: %d", ret);
        return ret;
    }

    /* 3. Find and load main script */
    LOG_INF("Loading main script...");
    const uint8_t *main_bytecode = hako_find_bytecode("main");
    if (!main_bytecode) {
        LOG_ERR("main.rb not found in registry");
        return -ENOENT;
    }

    ret = hako_load_bytecode("main", main_bytecode);
    if (ret < 0) {
        LOG_ERR("hako_load_bytecode failed: %d", ret);
        return ret;
    }

    /* 4. Execute Ruby code */
    LOG_INF("Executing Ruby application...");
    ret = hako_run();
    if (ret < 0) {
        LOG_ERR("hako_run failed: %d", ret);
        return ret;
    }

    LOG_INF("Ruby application completed successfully");
    return 0;
}
```

---

### Error Codes

| Error Code | Meaning |
|------------|---------|
| `0` | Success |
| `-ENOMEM` | Out of memory (increase `CONFIG_HAKO_MEMORY_SIZE`) |
| `-EINVAL` | Invalid bytecode or parameters |
| `-ENOENT` | Bytecode not found in registry |
| `-EFAULT` | VM execution error (Ruby exception) |

See `<hako/loader.h>` for complete API documentation.

## Requirements

### Software

**Required:**
- **Zephyr SDK 0.16.0+** - Embedded RTOS framework
- **West** - Zephyr build and flash tool
- **CMake 3.20+** - Build system

**Optional (for pre-compilation):**
- **mrbc** - Mruby bytecode compiler (for build-time Ruby compilation)

### Installing mrbc Compiler

The `mrbc` compiler is needed only if you want to pre-compile Ruby scripts at build time. Runtime compilation (on-device) doesn't need it.

**Option 1: Build from mruby source**
```bash
# Clone mruby
git clone https://github.com/mruby/mruby.git /tmp/mruby
cd /tmp/mruby

# Build (creates mrbc in build/host/bin/)
./minirake

# Add to PATH or set HAKO_MRBC_PATH
export PATH="/tmp/mruby/build/host/bin:$PATH"
# OR
export HAKO_MRBC_PATH=/tmp/mruby/build/host/bin/mrbc
```

**Option 2: Install via package manager**
```bash
# Ubuntu/Debian
sudo apt-get install mruby

# macOS
brew install mruby

# Arch Linux
sudo pacman -S mruby
```

**Verify installation:**
```bash
mrbc --version
# Output: mruby 3.x.x (2024-xx-xx)
```

### Hardware

| Configuration | Flash (ROM) | RAM | Use Case |
|---------------|-------------|-----|----------|
| **Minimal VM** | 128 KB | 64 KB | Production, pre-compiled bytecode only |
| **VM + Compiler** | 512 KB | 128 KB | Development, interactive shell |
| **Recommended** | 1 MB | 256 KB | Full features with headroom |
| **Maximum** | 2 MB+ | 512 KB+ | Large applications, multiple scripts |

**Recommended MCUs:**
- ARM Cortex-M4/M7 with FPU (STM32F4, STM32F7, NRF52, etc.)
- ARM Cortex-M33 (STM32H5, STM32U5)
- RISC-V RV32IMC or better
- x86/x64 (for QEMU testing)

### Tested Boards

The following boards have been tested with Hako:

- ✅ **qemu_x86** - QEMU x86 emulator (testing)
- Add your tested boards via pull request!

**Not Yet Tested (but should work):**
- STM32F4 Discovery
- STM32F7 Discovery
- Nordic nRF52840 DK
- ESP32 (with Zephyr support)
- Raspberry Pi Pico (RP2040)

## Performance

### Compilation Time
- Simple expression (10 tokens): ~10-50 ms
- Small script (50 lines): ~50-200 ms
- Medium script (200 lines): ~200-500 ms

### Execution Speed
- ~10-100x slower than native C
- Similar to interpreted Python
- Suitable for: Configuration, scripting, automation

## Creating Hako Extensions

Extend Hako with C-based Ruby modules to access hardware and system features.

### Extension Structure

```
extensions/my-extension/
├── CMakeLists.txt         # Build configuration
├── Kconfig                # Configuration options
├── include/
│   └── my_extension.h     # C API header
├── src/
│   └── my_extension.c     # C implementation
└── lib/
    └── my_module.rb       # Ruby bindings (optional)
```

### Example: Simple GPIO Extension

**1. Create Kconfig** (`extensions/my-gpio/Kconfig`):
```kconfig
config HAKO_MY_GPIO
    bool "MyGPIO Ruby Extension"
    depends on HAKO
    depends on GPIO
    help
      Provides GPIO control from Ruby.
```

**2. Implement C Extension** (`extensions/my-gpio/src/my_gpio.c`):
```c
#include <mrubyc.h>
#include <zephyr/drivers/gpio.h>

// C implementation
static void c_gpio_set(mrbc_vm *vm, mrbc_value *v, int argc) {
    if (argc != 2) {
        mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
        return;
    }

    int pin = GET_INT_ARG(1);
    int value = GET_INT_ARG(2);

    // Your GPIO logic here
    gpio_pin_set(gpio_dev, pin, value);

    SET_TRUE_RETURN();
}

// Register extension
void hako_my_gpio_init(mrbc_vm *vm) {
    mrbc_class *gpio_class = mrbc_define_class(vm, "GPIO", mrbc_class_object);
    mrbc_define_method(vm, gpio_class, "set", c_gpio_set);
}

// Auto-register using linker magic
HAKO_EXTENSION_DEFINE(my_gpio, hako_my_gpio_init);
```

**3. CMake Integration** (`extensions/my-gpio/CMakeLists.txt`):
```cmake
if(CONFIG_HAKO_MY_GPIO)
    zephyr_library_sources(src/my_gpio.c)
    zephyr_library_include_directories(include)
endif()
```

**4. Use in Ruby:**
```ruby
# No require needed - auto-loaded!
GPIO.set(13, 1)  # Turn on LED on pin 13
```

### Auto-Registration

Extensions are automatically registered at boot using the `HAKO_EXTENSION_DEFINE()` macro. The linker places all extensions in a special section that Hako loads during initialization.

See [extensions/zephyr-gpio/](extensions/zephyr-gpio/) for a complete example.

## Thread Safety

⚠️ **The Mruby/c VM and PicoRuby compiler are NOT thread-safe.**

### Thread Safety Guidelines

**Safe Patterns:**
- ✅ Execute all Ruby code in a single dedicated thread
- ✅ Use Zephyr message queues to send data to Ruby thread
- ✅ Use Zephyr mutexes if you must access VM from multiple threads
- ✅ Complete compilation before starting execution

**Unsafe Patterns:**
- ❌ Calling `mrbc_run()` from multiple threads simultaneously
- ❌ Compiling Ruby code while VM is executing (memory is the limit)
- ❌ Modifying VM state from interrupt handlers

**Recommended Architecture:**

```
┌──────────────────┐       ┌──────────────────┐
│  Sensor Thread   │       │  Network Thread  │
│                  │       │                  │
│  Read sensor     │       │  Receive data    │
│       ↓          │       │       ↓          │
│  Send to queue ──┼──┐    │  Send to queue ──┼──┐
└──────────────────┘  │    └──────────────────┘  │
                                            ↓                                                     ↓
           ┌──────────────────────────────────────┐
           │         Message Queue                │
           └──────────┬───────────────────────────┘
                                            ↓
               ┌──────────────────────────┐
               │   Ruby Thread (Main)     │
               │                          │
               │  Process messages        │
               │  Execute Ruby scripts    │
               │  Call C extensions       │
               └──────────────────────────┘
```

**Example:**

```c
// In main thread - safe
void main(void) {
    mrbc_init_alloc();
    mrbc_init_global();

    // Execute Ruby in main thread
    mrbc_eval("puts 'Hello from main thread'");
}

// In sensor thread - use message queue
K_MSGQ_DEFINE(sensor_msgq, sizeof(int), 10, 4);

void sensor_thread(void) {
    while (1) {
        int value = read_sensor();
        k_msgq_put(&sensor_msgq, &value, K_FOREVER);
    }
}

void main(void) {
    // Process in Ruby thread
    while (1) {
        int value;
        if (k_msgq_get(&sensor_msgq, &value, K_NO_WAIT) == 0) {
            // Safe: called from main thread
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "process_sensor(%d)", value);
            mrbc_eval(cmd);
        }
        k_sleep(K_MSEC(10));
    }
}
```

## Troubleshooting

### Build Errors

**"undefined reference to mrbc_init"**
- Ensure `CONFIG_HAKO=y` is set

**"undefined reference to fs_mkfs"**
- Add `CONFIG_FILE_SYSTEM_MKFS=y`

### Runtime Errors

**"Out of memory" during compilation**
- Increase `CONFIG_HAKO_COMPILER_MEMORY_SIZE`
- Increase `CONFIG_HEAP_MEM_POOL_SIZE`

**"Stack overflow" or page faults**
- Increase `CONFIG_MAIN_STACK_SIZE` to 4096 or higher

**"Failed to mount filesystem"**
- Check `CONFIG_FLASH_MAP=y` is set
- Verify storage partition exists in device tree
- For QEMU: Enable `CONFIG_FLASH_SIMULATOR=y`

## Contributing

Contributions are welcome! Please:

1. Follow Zephyr coding style
2. Test on at least one real board
3. Update documentation
4. Add Kconfig options for new features

## License

**Hako Module**: Licensed under the Apache License 2.0

This module integrates multiple open-source components, each with their own licenses:

| Component | License | Location |
|-----------|---------|----------|
| **Hako Core** | Apache-2.0 | All files except `ext/` directory |
| **Mruby/c VM** | BSD-3-Clause | `ext/mrubyc/` |
| **PicoRuby** | MIT | `ext/picoruby/` |
| **Prism Parser** | MIT | `ext/picoruby/mruby-compiler2/lib/prism/` |

See [LICENSE](LICENSE) for the full Apache-2.0 license text and individual component directories for their respective licenses.

## Credits and Acknowledgments

Hako is built upon excellent open-source projects:

### Core Components

**[Mruby/c](https://github.com/mrubyc/mrubyc)** - The VM Engine
- Ultra-lightweight Ruby bytecode interpreter
- Designed specifically for embedded systems
- Provides the runtime execution environment
- License: BSD-3-Clause

**[PicoRuby](https://github.com/picoruby/picoruby)** - Ruby for Microcontrollers
- Complete Ruby implementation for embedded devices
- Provides standard library gems (require, sandbox, etc.)
- Inspiration for package system design
- License: MIT

**[mruby-compiler2](https://github.com/picoruby/mruby-compiler2)** - Ruby Compiler
- Compiles Ruby source code to mruby bytecode
- Optimized for embedded systems
- Powers on-device compilation feature
- License: MIT

**[Prism](https://github.com/ruby/prism)** - Universal Ruby Parser
- Modern, portable Ruby parser
- Provides accurate AST generation
- Used by Ruby core team
- License: MIT

### Special Thanks

- **Zephyr Project** - For the excellent RTOS framework
- **Ruby Community** - For the beautiful language
- **mruby/c contributors** - For the lightweight VM
- **PicoRuby team** - For embedded Ruby ecosystem

## Support and Community

### Getting Help

- 📖 **Documentation**: See [docs/](docs/) directory for detailed guides
- 🐛 **Issues**: [Report bugs](https://github.com/your-org/hako/issues) and request features
- 💬 **Discussions**: [Ask questions](https://github.com/your-org/hako/discussions)
- 📧 **Contact**: For security issues, contact maintainers directly

### Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

**Ways to contribute:**
- Report bugs and issues
- Submit pull requests for features or fixes
- Improve documentation
- Add support for new boards
- Create extensions for common hardware
- Share your projects using Hako

## Roadmap

### Completed ✅
- [x] Mruby/c VM integration with Zephyr
- [x] PicoRuby compiler integration
- [x] Interactive shell (IRB-like) support
- [x] Filesystem support (LittleFS, FAT)
- [x] require/load support (picoruby-require)
- [x] Sandbox execution (picoruby-sandbox)
- [x] CMake build-time compilation utilities
- [x] Extension system with auto-registration
- [x] GPIO extension (Zephyr::GPIO)

### In Progress 🚧
- [ ] More hardware extensions (I2C, SPI, UART, etc.)
- [ ] Additional example applications
- [ ] Performance optimizations
- [ ] Better error messages and debugging

### Planned Features 📋
- [ ] Thread-safe VM option (mutex-protected operations)
- [ ] Debugging support (breakpoints, stepping, inspection)
- [ ] Performance profiling tools
- [ ] Remote script update mechanism (OTA updates)
- [ ] Integration with Zephyr logging and tracing subsystems
- [ ] Ruby bindings for common Zephyr APIs
- [ ] Package manager for Ruby gems
- [ ] More metaprogramming features

### Future Exploration 🔮
- [ ] Multi-VM support (isolated contexts)
- [ ] Foreign Function Interface (FFI)
- [ ] Visual programming interface for Ruby scripting

## See Also

- [Zephyr Project](https://www.zephyrproject.org/)
- [mruby](https://mruby.org/) - Original mruby project
- [Ruby Language](https://www.ruby-lang.org/)
