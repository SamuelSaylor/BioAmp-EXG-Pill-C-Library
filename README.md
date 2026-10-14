# bioampexgpillclib

While working on a separate project for BCIs, I realized that utilizing Python was not going to cut it in the long run. After a bit of development, I ended up creating a small C library for reading EEG/EXG data from an Upside Down Labs BioAmp EXG Pill through an ADS1115 ADC over I2C, on Linux (developed on a Raspberry Pi 4). I realized that UpsideDown Lab (the creators of BioAmp) does not seem to have a standard C library nor does one exist in the wild, so low and behold, the "first" C library for the product! I feel special already. 

It handles opening the I2C bus, configuring the ADS1115, reading raw samples, and converting them to volts. It does not do filtering, FFT, or classification — those live further up the stack, on top of this library.    

If you'd like to learn more about the BioAmp EXG Pill, refer to their official documentation here: https://docs.upsidedownlabs.tech/hardware/bioamp/bioamp-exg-pill/index.html   

## Hardware assumed

- BioAmp EXG Pill wired to an ADS1115's `A0` input, configured for EEG/EOG mode
- ADS1115 on I2C bus `/dev/i2c-1`, default address `0x48` (ADDR pin tied to GND)
- ADS1115 configured for: continuous conversion mode, single-ended channel A0, ±4.096V PGA range, 128 SPS

If you wire it differently (different channel, different PGA range, different data rate), you'll need to change the config bytes in `bioampexgpillclib_init()` and the `LSB` constant to match — see **Function reference** below for what those values mean.
*Please be very careful when wiring/soldering. It is easy to short-circuit your beautiful BioAmp EXG Pill if you do the wrong thing.*   

<img width="5712" height="4284" alt="image0 (2)" src="https://github.com/user-attachments/assets/1908a04d-d4eb-4ef7-8423-c143453147d1" />
Forgive me for the sloppy camera work- the wiring that feeds data from the human to the EXG Pill is a bit long and squiggly.   

## Building

```
make
```

This builds `libbioampexgpillclib.a` (the static library) and `test_main` (an example program that uses it).

```
make clean
```

removes the compiled `.o`, `.a`, and binary files.

## Using it in your own project

You need two things: `bioampexgpillClib.h` and `libbioampexgpillclib.a`. Copy or reference both, then compile your program against them:

```
gcc yourprogram.c -I/path/to/header -L/path/to/libfolder -lbioampexgpillclib -o yourprogram
```

Minimal usage example:

```c
#include "bioampexgpillClib.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int fd = bioampexgpillclib_init("/dev/i2c-1", 0x48);
    if (fd < 0) {
        printf("init failed\n");
        return -1;
    }

    int16_t raw;
    while (bioampexgpillclib_read_raw(fd, &raw) == 0) {
        double voltage = bioampexgpillclib_counts_to_voltage(raw);
        printf("raw: %d, voltage: %.4f V\n", raw, voltage);
        usleep(100000);
    }

    bioampexgpillclib_close(fd);
    return 0;
}
```

## Function reference

### `int bioampexgpillclib_init(const char *i2c_device, int addr)`

Opens the I2C bus, selects the ADS1115 at the given address, and writes the config register to start continuous conversion mode.

- `i2c_device` — path to the I2C bus device, e.g. `"/dev/i2c-1"`.
- `addr` — the ADS1115's I2C address, e.g. `0x48`.
- **Returns:** an open file descriptor (>= 0) on success, or `-1` if the bus couldn't be opened, the device didn't respond, or the config write failed.

The returned file descriptor is what you pass into every other function below — treat it as an opaque handle, not a value to inspect or modify yourself.

### `int bioampexgpillclib_read_raw(int i2c_file, int16_t *out_raw)`

Reads one sample from the ADS1115's conversion register.

- `i2c_file` — the file descriptor returned by `bioampexgpillclib_init()`.
- `out_raw` — pointer to an `int16_t` that receives the raw signed ADC count on success.
- **Returns:** `0` on success, `-1` if either the register-select write or the 2-byte read failed. On failure, `*out_raw` is left unchanged — don't trust its contents unless the return value was `0`.

Each call blocks briefly on the I2C transaction but does not itself wait for a new conversion — in continuous mode the ADS1115 is always updating this register in the background at the configured data rate (128 SPS by default), so calling this faster than that rate will sometimes return the same value twice.

### `double bioampexgpillclib_counts_to_voltage(int16_t raw)`

Converts a raw ADC count into volts.

- `raw` — a raw count, typically fresh from `bioampexgpillclib_read_raw()`.
- **Returns:** the corresponding voltage as a `double`, using the fixed scale factor `LSB = 4.096 / 32768.0` (volts per count for the ±4.096V PGA range).

This is a pure function — no I2C, no side effects, safe to call as often as you like and easy to unit-test on its own with known inputs.

If you reconfigure the ADS1115 for a different PGA range, update the `LSB` constant in `bioampexgpillClib.c` to match, or this function will silently return the wrong voltage.

### `void bioampexgpillclib_close(int i2c_file)`

Closes the I2C file descriptor.

- `i2c_file` — the file descriptor returned by `bioampexgpillclib_init()`.
- **Returns:** nothing. Call this once you're done sampling; using `i2c_file` in any other function afterward is undefined behavior.

## Known limitations / next steps

- Single channel only (A0) — no multi-channel support yet.
- No internal buffering — each `read_raw()` call is a fresh I2C transaction; callers needing a steady sample rate are responsible for their own timing loop.
- Config register values are hardcoded in `bioampexgpillclib_init()` rather than exposed as parameters — changing gain/rate/channel currently means editing the source.
