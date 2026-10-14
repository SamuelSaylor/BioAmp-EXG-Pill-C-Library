#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "bioampexgpillClib.h"

#define REG_CONVERSION 0x00
#define REG_CONFIG 0x01

#define LSB (4.096/32768.0) //lsb = least significant bit

// retry a syscall that was interrupted by a signal
static ssize_t write_retry(int fd, const void *buf, size_t len){
	ssize_t r;
	do { r = write(fd, buf, len); } while (r < 0 && errno == EINTR);
	return r;
}

static ssize_t read_retry(int fd, void *buf, size_t len){
	ssize_t r;
	do { r = read(fd, buf, len); } while (r < 0 && errno == EINTR);
	return r;
}

int bioampexgpillclib_init(const char *i2c_device, int addr){
	int i2c_file = open(i2c_device, O_RDWR);

	if (i2c_file < 0) return -errno;

	if (ioctl(i2c_file, I2C_SLAVE, addr) < 0) {//additionally ioctl is i/o control. its not literally controlling ur i2c bus, but it is a way to send commands to the i2c driver in linux. in this case, we are telling the driver that we want to talk to the ADS1115 at address 0x48
		int err = errno;
		close(i2c_file);
		return -err;
	}

	// config: AIN0 vs GND, PGA = +/-4.096V (bits 11:9 = 001), continuous mode, 128 SPS, comparator off
	unsigned char config_hi = 0xC2;
	unsigned char config_lo = 0x83;
	unsigned char config_write[3] = { REG_CONFIG, config_hi, config_lo };
	if (write_retry(i2c_file, config_write, 3) != 3) {
		int err = errno ? errno : EIO;
		close(i2c_file);
		return -err;
	}

	// point at the conversion register once. in continuous mode the pointer sticks,
	// so reads only need a single read() syscall instead of write()+read()
	unsigned char reg_pointer = REG_CONVERSION;
	if (write_retry(i2c_file, &reg_pointer, 1) != 1) {
		int err = errno ? errno : EIO;
		close(i2c_file);
		return -err;
	}

	usleep(10000); //10ms, lets the first conversion finish
	return i2c_file;
}

int bioampexgpillclib_read_raw(int i2c_file, int16_t *out_raw){
	unsigned char buf[2];
	ssize_t r = read_retry(i2c_file, buf, 2);
	if (r < 0) return -errno;
	if (r != 2) return -EIO;

	*out_raw = (int16_t)((buf[0] << 8) | buf[1]);
	return 0;
}

int bioampexgpillclib_read_block(int i2c_file, int16_t *out_raw, size_t n, double rate_hz){
	struct timespec next;
	long period_ns = rate_hz > 0 ? (long)(1e9 / rate_hz) : 0;
	if (period_ns) clock_gettime(CLOCK_MONOTONIC, &next);

	for (size_t i = 0; i < n; i++) {
		if (period_ns) {
			// absolute deadlines, so jitter/read time doesn't accumulate as drift
			next.tv_nsec += period_ns;
			while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
			while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
		}
		int rc = bioampexgpillclib_read_raw(i2c_file, &out_raw[i]);
		if (rc < 0) return rc;
	}
	return 0;
}

double bioampexgpillclib_counts_to_voltage(int16_t raw){return raw * LSB;} //this is volts-per-count for +/-4.096 range, id change LSB if you change the config register
/*
in regards to int16_t as it made me curious when i was researching how to build something like this-

int16_t is a data type that means a signed integer that is EXACTLY 16 bits wide. this is essential because
i used the ADS1115 in a +/-4.096V range, which is 2^15 counts in each direction, so the raw counts are a
signed 16 bit integer.
*/

int bioampexgpillclib_close(int i2c_file){return close(i2c_file) < 0 ? -errno : 0;}

/*
If you'd like to test it yourself!

int main(void) {
    int i2c_file = bioampexgpillclib_init("/dev/i2c-1", 0x48);
    if(i2c_file<0){
		printf("Failed to initialize bioampexgpillclib: %d\n", i2c_file);
		return -1;
	}

	int16_t raw;
	while(1){
		if(bioampexgpillclib_read_raw(i2c_file,&raw)<0){
			printf("Failed to read raw data.\n");
			break;
		}

		double voltage = bioampexgpillclib_counts_to_voltage(raw);
		printf("raw counts: %d, voltage: %.4f V\n", raw, voltage);

        usleep(100000);
	}

    bioampexgpillclib_close(i2c_file);
    return 0;
}
*/
