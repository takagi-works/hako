# SPDX-License-Identifier: Apache-2.0
# Zephyr::GPIO Ruby sugar layer

module Zephyr
  class GPIO
    # Convenience methods for digital output

    def on
      write(1)
    end

    def off
      write(0)
    end

    # Query methods

    def high?
      read == 1
    end

    def low?
      read == 0
    end

    # Toggle multiple times (blink pattern)
    def blink(times = 5)
      times.times do
        toggle
        # Note: would need sleep() implementation
        # k_msleep equivalent from Ruby
      end
    end
  end
end
