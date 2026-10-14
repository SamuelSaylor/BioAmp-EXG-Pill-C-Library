#ifndef BIOAMPEXGPILLCLIB_H
#define BIOAMPEXGPILLCLIB_H

#include <stddef.h>
#include <stdint.h>

/* All functions return 0 / a valid value on success and -errno on failure. */

int bioampexgpillclib_init(const char *i2c_device, int addr); /* returns fd >= 0 */
int bioampexgpillclib_read_raw(int i2c_file, int16_t *out_raw);
/* Reads n samples, paced at `rate_hz` samples/s (0 = as fast as possible). */
int bioampexgpillclib_read_block(int i2c_file, int16_t *out_raw, size_t n, double rate_hz);
double bioampexgpillclib_counts_to_voltage(int16_t raw);
int bioampexgpillclib_close(int i2c_file);

#endif // BIOAMPEXGPILLCLIB_H
