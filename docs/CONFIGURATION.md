# Hako Module Configuration Guide

This document provides detailed information about all configuration options available for the Hako Zephyr module.

## Quick Start Configurations

### Minimal VM Only (40 KB ROM, 40 KB RAM)
```ini
CONFIG_HAKO=y
```
Use this for production with pre-compiled bytecode only.

### Full Development (500-700 KB ROM, 105 KB RAM)
```ini
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_COMPILER_MEMORY_SIZE=65536
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_SHELL=y
CONFIG_HEAP_MEM_POOL_SIZE=131072
```
Use this for interactive development and testing.

### Full with Filesystem (500-700 KB ROM, 121 KB RAM)
```ini
CONFIG_HAKO=y
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_COMPILER_MEMORY_SIZE=65536
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_HAKO_FILE_SUPPORT=y

CONFIG_SHELL=y
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FS_LITTLEFS_FMP_DEV=y

CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
```
Use this for loading Ruby scripts from persistent storage.

## Core Configuration Options

### CONFIG_HAKO
```
Type: bool
Default: n
Dependencies: None
```

**Description**: Main toggle for Hako module support.

**Enables**:
- Hako VM core
- Basic Ruby execution
- Core Ruby classes (String, Array, Hash, Integer, Float, etc.)

**ROM Impact**: ~40 KB
**RAM Impact**: Controlled by `CONFIG_HAKO_MEMORY_POOL_SIZE`

**Usage**:
```ini
CONFIG_HAKO=y
```

### CONFIG_HAKO_MEMORY_POOL_SIZE
```
Type: int
Default: 40960 (40 KB)
Range: 8192 - 262144
Dependencies: CONFIG_HAKO=y
```

**Description**: Size of the Hako VM memory pool in bytes.

**What it controls**:
- Ruby object allocation
- VM internal data structures
- String/Array/Hash storage
- Call stack depth

**Guidelines**:
- **Minimum**: 8 KB (very limited, ~100 objects)
- **Small applications**: 20-40 KB
- **Medium applications**: 40-80 KB
- **Large applications**: 80-256 KB

**Example**:
```ini
# 64 KB VM pool for larger scripts
CONFIG_HAKO_MEMORY_POOL_SIZE=65536
```

**Note**: This memory is permanently allocated at boot.

## Compiler Subsystem Options

### CONFIG_HAKO_COMPILER
```
Type: bool (menuconfig)
Default: n
Dependencies: CONFIG_SHELL=y
```

**Description**: Enable PicoRuby compiler subsystem for on-device compilation.

**Enables**:
- mruby-compiler2 code generator
- Prism parser
- Runtime Ruby source → bytecode compilation
- Interactive Ruby execution

**ROM Impact**: +250-500 KB
**RAM Impact**: Controlled by `CONFIG_HAKO_COMPILER_MEMORY_SIZE`

**Usage**:
```ini
CONFIG_HAKO_COMPILER=y
```

**When to use**:
- Development and testing
- Interactive scripting
- Dynamic Ruby code execution
- Prototyping

**When NOT to use**:
- Production with fixed scripts (use pre-compiled bytecode)
- Extreme memory constraints
- When code size > 500 KB is too large

### CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE
```
Type: bool
Default: y
Dependencies: CONFIG_HAKO_COMPILER=y
```

**Description**: Optimize compiler for size instead of features.

**When enabled**:
- Minimal Prism parser (~200 KB)
- Reduced error messages
- Limited debug information
- **ROM savings**: ~100-200 KB

**When disabled**:
- Full Prism parser (~400 KB)
- Detailed error messages
- Full debug information
- Better diagnostics

**Usage**:
```ini
# Optimize for size (recommended)
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y

# Optimize for debugging
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=n
```

**Recommendation**: Keep enabled (y) for production.

### CONFIG_HAKO_COMPILER_MEMORY_SIZE
```
Type: int
Default: 65536 (64 KB)
Range: 32768 - 262144
Dependencies: CONFIG_HAKO_COMPILER=y
```

**Description**: Memory pool size for compiler (separate from VM pool).

**What it controls**:
- AST (Abstract Syntax Tree) storage
- IR (Intermediate Representation) generation
- Symbol table
- Maximum compilable script size

**Guidelines**:
- **Minimum**: 32 KB (small scripts, <50 lines)
- **Small scripts**: 64 KB (default, <200 lines)
- **Medium scripts**: 128 KB (<500 lines)
- **Large scripts**: 256 KB (>500 lines)

**Example**:
```ini
# For larger scripts
CONFIG_HAKO_COMPILER_MEMORY_SIZE=131072
```

**Important**: This memory is only allocated during compilation, then freed.

### CONFIG_HAKO_EVAL
```
Type: bool
Default: n
Dependencies: CONFIG_HAKO_COMPILER=y
```

**Description**: Enable Ruby `eval()` function support.

**Provides**:
- `eval(string)` method in Ruby
- Runtime code evaluation
- Programmatic compilation from Ruby

**ROM Impact**: Minimal (~1 KB)
**RAM Impact**: None (uses compiler pool)

**Usage**:
```ini
CONFIG_HAKO_EVAL=y
```

**Ruby example**:
```ruby
code = "puts 'Hello from eval!'"
eval(code)
```

**Security note**: Be cautious with eval() - only use with trusted input.

## Shell Integration Options

### CONFIG_HAKO_IRB_COMMAND
```
Type: bool
Default: n
Dependencies: CONFIG_HAKO_EVAL=y, CONFIG_SHELL=y
```

**Description**: Enable Ruby shell commands (ruby, ruby_info, ruby_file).

**Provides shell commands**:
- `ruby <expression>` - Execute Ruby code
- `ruby_info` - Show compiler information
- `ruby_file <path>` - Load and execute Ruby file (requires CONFIG_HAKO_FILE_SUPPORT)

**ROM Impact**: ~2 KB
**RAM Impact**: 512 bytes (command buffers)

**Usage**:
```ini
CONFIG_HAKO_IRB_COMMAND=y
```

**Shell examples**:
```bash
uart:~$ ruby "puts 'Hello!'"
uart:~$ ruby "x = 10; puts x * 2"
uart:~$ ruby_info
uart:~$ ruby_file /lfs/script.rb
```

### CONFIG_HAKO_FILE_SUPPORT
```
Type: bool
Default: n
Dependencies: CONFIG_HAKO_IRB_COMMAND=y, CONFIG_FILE_SYSTEM=y
```

**Description**: Enable loading Ruby scripts from filesystem.

**Provides**:
- `ruby_file <path>` shell command
- File loading API (if exposing C API)

**Requirements**:
- Zephyr filesystem subsystem enabled
- Mounted filesystem (LittleFS, FAT, etc.)

**ROM Impact**: ~1 KB
**RAM Impact**: 16 KB buffer during file load

**Usage**:
```ini
CONFIG_HAKO_FILE_SUPPORT=y
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y  # Or FAT, NVS, etc.
```

**Max file size**: 16 KB (defined in shell_irb.c `MAX_FILE_SIZE`)

## Memory Configuration

### CONFIG_HEAP_MEM_POOL_SIZE
```
Type: int
Default: varies by board
Dependencies: None (Zephyr core)
```

**Description**: Zephyr system heap size.

**What uses it**:
- Hako VM pool allocation
- Compiler pool allocation (during compilation)
- File buffers (during file loading)
- Shell buffers
- Application allocations

**Guidelines for Hako**:

| Configuration | Recommended Heap |
|--------------|------------------|
| VM only | 65536 (64 KB) |
| VM + Compiler | 131072 (128 KB) |
| VM + Compiler + Files | 131072 (128 KB) |

**Calculation**:
```
Heap Size = VM Pool + Compiler Pool + File Buffer + 20% margin

Example (Full config):
  = 40 KB + 64 KB + 16 KB + 24 KB
  = 144 KB
  → Use 131072 (128 KB) minimum, 163840 (160 KB) recommended
```

**Usage**:
```ini
CONFIG_HEAP_MEM_POOL_SIZE=131072
```

### CONFIG_MAIN_STACK_SIZE
```
Type: int
Default: varies by board (typically 1024-2048)
Dependencies: None (Zephyr core)
```

**Description**: Main thread stack size.

**What uses it**:
- Ruby compilation (recursive parsing)
- Ruby execution (call stack)
- Shell command processing

**Guidelines**:
- **VM only**: 2048 bytes
- **VM + Compiler**: 4096 bytes (recommended)
- **Large scripts**: 8192 bytes

**Usage**:
```ini
CONFIG_MAIN_STACK_SIZE=4096
```

**Stack overflow symptoms**:
- Crashes during compilation
- Page faults in parser
- Unpredictable behavior with deep recursion

### CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE
```
Type: int
Default: varies by board
Dependencies: None (Zephyr core)
```

**Description**: System work queue stack size.

**Recommendation for Hako**:
```ini
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048
```

Increase if using work queue for Ruby tasks.

## Shell Configuration (Required for IRB)

### CONFIG_SHELL
```
Type: bool
Default: n
```

**Description**: Enable Zephyr shell subsystem.

**Required for**:
- `ruby`, `ruby_file`, `ruby_info` commands
- Interactive Ruby execution

**Usage**:
```ini
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_SERIAL=y  # For UART shell
```

### CONFIG_SHELL_CMD_BUFF_SIZE
```
Type: int
Default: 256
```

**Description**: Shell command buffer size.

**Recommendation for Ruby**:
```ini
CONFIG_SHELL_CMD_BUFF_SIZE=256  # For short expressions
CONFIG_SHELL_CMD_BUFF_SIZE=512  # For longer expressions
```

**Note**: For scripts >512 bytes, use `ruby_file` instead.

### CONFIG_SHELL_HISTORY
```
Type: bool
Default: n
```

**Description**: Enable command history.

**Useful for**:
- Repeating Ruby commands
- Development workflow

**Usage**:
```ini
CONFIG_SHELL_HISTORY=y
CONFIG_SHELL_HISTORY_BUFFER=512
```

## Filesystem Configuration (Optional)

### For LittleFS (Recommended for Flash)

```ini
# Filesystem core
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_MKFS=y

# LittleFS
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FS_LITTLEFS_FMP_DEV=y  # Use flash map partition
CONFIG_FS_LITTLEFS_NUM_FILES=10
CONFIG_FS_LITTLEFS_NUM_DIRS=10

# Flash support
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y

# For QEMU testing
CONFIG_FLASH_SIMULATOR=y
```

### For FAT (For SD Cards)

```ini
# Filesystem core
CONFIG_FILE_SYSTEM=y

# FAT
CONFIG_FAT_FILESYSTEM_ELM=y

# SD card (adjust for your board)
CONFIG_DISK_ACCESS=y
CONFIG_DISK_ACCESS_SD=y
CONFIG_SPI=y
```

## Configuration Templates

### Template 1: Production (VM Only, Pre-compiled Bytecode)

**Use case**: Deploy with fixed, pre-compiled Ruby scripts

```ini
# Core
CONFIG_HAKO=y
CONFIG_HAKO_MEMORY_POOL_SIZE=40960

# Memory
CONFIG_HEAP_MEM_POOL_SIZE=65536
CONFIG_MAIN_STACK_SIZE=2048

# Size: ~40 KB ROM, ~40 KB RAM
```

### Template 2: Development (Interactive REPL)

**Use case**: Interactive development, testing, debugging

```ini
# Core
CONFIG_HAKO=y
CONFIG_HAKO_MEMORY_POOL_SIZE=40960

# Compiler
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_COMPILER_MEMORY_SIZE=65536
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y

# Shell
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_SERIAL=y
CONFIG_SHELL_CMD_BUFF_SIZE=256
CONFIG_SHELL_HISTORY=y
CONFIG_SHELL_HISTORY_BUFFER=512

# Memory
CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

# Size: ~500 KB ROM, ~105 KB peak RAM
```

### Template 3: Application Scripts (Filesystem-based)

**Use case**: Load Ruby applications from persistent storage

```ini
# Core
CONFIG_HAKO=y
CONFIG_HAKO_MEMORY_POOL_SIZE=40960

# Compiler
CONFIG_HAKO_COMPILER=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_HAKO_COMPILER_MEMORY_SIZE=65536
CONFIG_HAKO_EVAL=y
CONFIG_HAKO_IRB_COMMAND=y
CONFIG_HAKO_FILE_SUPPORT=y

# Shell
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_SERIAL=y

# Filesystem (LittleFS example)
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_MKFS=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FS_LITTLEFS_FMP_DEV=y
CONFIG_FS_LITTLEFS_NUM_FILES=10
CONFIG_FS_LITTLEFS_NUM_DIRS=10

# Flash
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FLASH_SIMULATOR=y  # For QEMU, remove for real hardware

# Memory
CONFIG_HEAP_MEM_POOL_SIZE=131072
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

# Size: ~500 KB ROM, ~121 KB peak RAM
```

## Configuration Decision Tree

```
Do you need Ruby at runtime?
├─ No → Don't enable CONFIG_HAKO
└─ Yes → CONFIG_HAKO=y
    │
    Do you need to compile Ruby source code on-device?
    ├─ No (pre-compiled bytecode only) → VM Only mode
    │   └─ ROM: ~40 KB, RAM: ~40 KB
    │
    └─ Yes → CONFIG_HAKO_COMPILER=y
        │
        Do you need to load Ruby scripts from files?
        ├─ No (expressions only) → Development mode
        │   ├─ CONFIG_HAKO_EVAL=y
        │   ├─ CONFIG_HAKO_IRB_COMMAND=y
        │   └─ ROM: ~500 KB, RAM: ~105 KB
        │
        └─ Yes → Full mode
            ├─ CONFIG_HAKO_FILE_SUPPORT=y
            ├─ CONFIG_FILE_SYSTEM=y
            └─ ROM: ~500 KB, RAM: ~121 KB
```

## Troubleshooting Configuration Issues

### "CONFIG_HAKO_COMPILER=y but got n"
**Cause**: Missing dependency `CONFIG_SHELL=y`
**Solution**: Add `CONFIG_SHELL=y`

### "CONFIG_HAKO_FILE_SUPPORT=y but got n"
**Cause**: Missing `CONFIG_FILE_SYSTEM=y` or `CONFIG_HAKO_IRB_COMMAND=y`
**Solution**: Enable both dependencies

### "Out of memory during compilation"
**Cause**: `CONFIG_HAKO_COMPILER_MEMORY_SIZE` too small
**Solution**: Increase to 131072 (128 KB) or higher

### "Stack overflow" or "Page fault" during compilation
**Cause**: `CONFIG_MAIN_STACK_SIZE` too small
**Solution**: Increase to at least 4096

### "Failed to mount filesystem"
**Cause**: Flash partition not configured or `CONFIG_FLASH_MAP` missing
**Solution**:
- Add `CONFIG_FLASH=y` and `CONFIG_FLASH_MAP=y`
- Verify device tree has storage partition
- For QEMU: `CONFIG_FLASH_SIMULATOR=y`

### Build fails with "undefined reference to fs_mkfs"
**Cause**: Missing `CONFIG_FILE_SYSTEM_MKFS=y`
**Solution**: Add `CONFIG_FILE_SYSTEM_MKFS=y`

## Performance Tuning

### Optimize for Size
```ini
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=y
CONFIG_LTO=y  # Link Time Optimization
```

### Optimize for Speed
```ini
CONFIG_SPEED_OPTIMIZATIONS=y
CONFIG_HAKO_COMPILER_OPTIMIZE_SIZE=n
```

### Optimize for Memory
```ini
# Minimize VM pool
CONFIG_HAKO_MEMORY_POOL_SIZE=20480  # 20 KB

# Minimize compiler pool
CONFIG_HAKO_COMPILER_MEMORY_SIZE=32768  # 32 KB

# Minimize heap
CONFIG_HEAP_MEM_POOL_SIZE=65536  # 64 KB
```

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture overview
- [FILE_SUPPORT.md](../FILE_SUPPORT.md) - Using filesystem with Ruby scripts
- [Zephyr Kconfig Documentation](https://docs.zephyrproject.org/latest/build/kconfig/index.html)
