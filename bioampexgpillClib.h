#ifndef BIOAMPEXGPILLCLIB_H
#define BIOAMPEXGPILLCLIB_H

#include <stdint.h>

int bioampexgpillclib_init(const char *i2c_device, int addr);
int bioampexgpillclib_read_raw(int i2c_file, int16_t *out_raw);
double bioampexgpillclib_counts_to_voltage(int16_t raw);
void bioampexgpillclib_close(int i2c_file);

#endif // BIOAMPEXGPILLCLIB_H