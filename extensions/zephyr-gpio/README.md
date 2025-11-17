# Zephyr::GPIO Extension

Ruby API for controlling GPIO pins on Zephyr RTOS.

## Usage

```ruby
# Open LED GPIO
led = Zephyr::GPIO.open(13, mode: :output)

# Basic operations
led.write(1)        # Turn on
led.write(0)        # Turn off
led.toggle          # Toggle state
value = led.read    # Read current value (0 or 1)

# Convenience methods (from Ruby sugar layer)
led.on              # Same as write(1)
led.off             # Same as write(0)
led.high?           # Returns true if pin is high
led.low?            # Returns true if pin is low

# Blink pattern
led.blink(5)        # Toggle 5 times
```

## Configuration

Enable in `prj.conf`:

```conf
CONFIG_HAKO_ZEPHYR_GPIO=y
CONFIG_GPIO=y
```

## Implementation Status

- [x] Basic C binding structure
- [x] Ruby sugar layer
- [ ] Full devicetree alias support
- [ ] Actual Zephyr GPIO API integration
- [ ] Input with pull-up/pull-down
- [ ] Interrupt support

This is a **scaffolding example** - GPIO operations are logged but not yet
connected to actual hardware. Next steps:

1. Integrate with Zephyr `gpio_dt_spec` API
2. Support devicetree aliases (`:led0`, `:button0`, etc.)
3. Add proper error handling
4. Implement all GPIO modes (input, output, etc.)
