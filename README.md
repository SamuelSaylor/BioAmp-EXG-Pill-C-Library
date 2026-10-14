# bioampexgpillclib

For all information regarding the C-library, refer to README.md on the main branch. Thank you.   


## Python usage

The library is also available on PyPI as `bioampexgpillclib`. The Python package wraps the C code in a small C extension, so reads run at C speed and the GIL is released while talking to the I2C bus. It is **Linux only** (it needs `/dev/i2c-*`), and it is an early release that has not yet been tested on real hardware, so please report any problems you hit.

### Installing

```
python3 -m venv .venv
. .venv/bin/activate
pip install bioampexgpillclib
```

If no prebuilt wheel matches your system, pip builds from source, which needs a compiler and the Python headers (on Raspberry Pi OS: `sudo apt install python3-dev build-essential`). You also need I2C enabled (`raspi-config`) and your user in the `i2c` group, otherwise opening the bus raises `PermissionError`.

### Quick start

```python
from bioampexgpillclib import BioAmp

with BioAmp("/dev/i2c-1", 0x48) as amp:
    print(amp.read_voltage())             # one sample, in volts
    samples = amp.read_block(1280, volts=True)  # 10 seconds at 128 SPS
```

### API reference

#### `BioAmp(device="/dev/i2c-1", address=0x48)`

Opens the I2C bus and configures the ADS1115 (continuous mode, channel A0, ±4.096V, 128 SPS). Raises `OSError` (or a subclass such as `PermissionError` or `FileNotFoundError`) if the bus can't be opened or the device doesn't respond. Works as a context manager, which closes the bus for you.

#### `read_raw() -> int`

One raw signed 16-bit ADC count.

#### `read_voltage() -> float`

One sample converted to volts.

#### `read_block(n, volts=False, rate=128) -> array.array`

Reads `n` samples in a single call and returns a standard-library `array`: `array('h')` of raw counts, or `array('d')` of volts when `volts=True`. The whole loop runs in C, which is much faster than calling `read_raw()` in a Python loop.

- `rate` — the pace in samples per second (default `128`, matching the ADS1115 data rate). Pass `0` to read as fast as possible, but note the ADC only produces a new value 128 times a second, so you will get repeated values.
- Because the reads are paced, `read_block(n)` takes about `n / rate` seconds.

#### `close()`

Closes the I2C bus. Safe to call more than once.

### Notes

- Any I2C failure raises `OSError` with the underlying `errno`, instead of printing a message like the C functions do.
- `array` objects work with `numpy.frombuffer(samples, dtype=...)` if you want NumPy, but NumPy is not required.
- Source, wiring notes and the standalone C library documentation: https://github.com/SamuelSaylor/BioAmp-EXG-Pill-C-Library
