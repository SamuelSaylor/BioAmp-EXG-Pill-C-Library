#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "bioampexgpillClib.h"

#define REG_CONVERSION 0x00
#define REG_CONFIG 0x01

#define LSB (4.096/32768.0) //lsb = least significant bit

int bioampexgpillclib_init(const char *i2c_device, int addr){
	int i2c_file = open(i2c_device, O_RDWR);
	
	if (i2c_file < 0) {
		printf("bioampexgpillclib: Failed to open the i2c bus.\n");
        return -1;
	}

	if (ioctl(i2c_file, I2C_SLAVE, addr) < 0) {//additionally ioctl is i/o control. its not literally controlling ur i2c bus, but it is a way to send commands to the i2c driver in linux. in this case, we are telling the driver that we want to talk to the ADS1115 at address 0x48
        printf("Failed to acquire bus access and/or talk to slave.\n");
        close(i2c_file);
        return -1;
	}

	unsigned char config_hi = 0xC4;
	unsigned char config_lo = 0x83;
	unsigned char config_write[3] = { REG_CONFIG, config_hi, config_lo };
	if (write(i2c_file, config_write, 3) != 3) {
		printf("Failed to write config register.\n");
        close(i2c_file);
        return -1;
	}

	usleep(10000); //10ms
	return i2c_file;
};

int bioampexgpillclib_read_raw(int i2c_file, int16_t *out_raw){
	// ---- SELECT CONVERSION REGISTER ----
    unsigned char reg_pointer = REG_CONVERSION;
    if (write(i2c_file, &reg_pointer, 1) != 1) {printf("Failed to select conversion register.\n");return -1;}

    // ---- READ 2 BYTES ----
    unsigned char buf[2];
    if (read(i2c_file, buf, 2) != 2) {printf("Failed to read conversion register.\n");return -1;}

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *out_raw = raw;
    return 0;
};

double bioampexgpillclib_counts_to_voltage(int16_t raw){return raw * LSB;} //this is volts-per-count for +/-4.096 range, id change LSB if you change the config register
/*
in regards to int16_t as it made me curious when i was researching how to build something like this-

int16_t is a data type that means a signed integer that is EXACTLY 16 bits wide. this is essential because
i used the ADS1115 in a +/-4.096V range, which is 2^15 counts in each direction, so the raw counts are a
signed 16 bit integer.
*/

void bioampexgpillclib_close(int i2c_file){close(i2c_file);};

/*
If you'd like to test it yourself!

int main(void) {
    int i2c_file = bioampexgpillclib_init("/dev/i2c-1", 0x48);
    if(i2c_file<0){
		printf("Failed to initialize bioampexgpillclib.\n");
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