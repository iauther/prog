#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <windows.h>

#include "sf.h"
#include "ch341a.h"

bool sf_init()
{
	int r;
	int cap;
	uint32_t speed = CH341A_STM_I2C_20K;

	r = ch341Configure(CH341A_USB_VENDOR, CH341A_USB_PRODUCT);
	if (r < 0) {
		fprintf(stderr, "ch341Configure failed\n");
		return false;
	}

	printf("____ 111\n");
	r = ch341SetStream(speed);
	if (r < 0) {
		fprintf(stderr, "ch341SetStream failed\n");
		return false;
	}

	printf("____ 222\n");
	r = ch341SpiCapacity();
	if (r < 0) {
		fprintf(stderr, "ch341SpiCapacity failed\n");
		return false;
	}

	printf("____ 333\n");
	cap = 1 << r;
	printf("Chip capacity is %d bytes\n", cap);

	return true;
}


bool sf_free()
{
	ch341Release();
	return true;
}


bool sf_erase()
{
	int r;
	uint8_t timeout = 0;

	r = ch341EraseChip();
	if (r < 0) {
		fprintf(stderr, "Chip erase error 1\n");
		return false;
	}

	do {
		Sleep(20);
		r = ch341ReadStatus();
		if (r < 0) {
			fprintf(stderr, "Chip erase error 2\n");
			return false;
		}

		timeout++;
		if (timeout == 100) break;
	} while (r != 0);

	if (timeout == 100)
		fprintf(stderr, "Chip erase timeout.\n");
	else
		printf("Chip erase done!\n");

	return true;
}



 bool sf_write(uint32_t addr, uint8_t *data, int32_t len)
{
	int r;

	r = ch341SpiWrite(addr, data, len);
	if (r >= 0) {
		return true;
	}

	return false;
}


bool sf_read(uint32_t addr, uint8_t *data, int32_t len)
{
	int r;

	r = ch341SpiRead(addr, data, len);
	if (r >= 0) {
		return true;
	}
	else {
		printf("____ sf_read ret: %d\n", r);
	}

	return false;
}


bool sf_save(int8_t *name, uint8_t *buf, int32_t len)
{
	bool r = true;
	FILE *file;
	size_t wlen;

	if (!name || !buf || !len) {
		return false;
	}

	file = fopen(name, "wb");
	if (!file) {
		printf("____ uboot open failed\n");
		return false;
	}

	wlen = fwrite((void*)buf, 1, len, file);
	if (wlen != len) {
		r = false;
	}

	fclose(file);

	return r;
}

//return same data number
static int32_t my_memcmp(uint8_t *m1, uint8_t *m2, int32_t len)
{
	int32_t i,l=0;
	for (i=0;i<len;i++) {
		if (m1[i] != m2[i]) {
			break;
		}
		l++;
	}

	return l;
}

bool sf_check(uint32_t addr, uint8_t *dat, int32_t len)
{
	bool r;
	uint8_t *tmp;

	if (!dat || !len) {
		return false;
	}

	tmp = (uint8_t *)malloc(len);
	if (!tmp) {
		return false;
	}

	r = sf_read(addr, tmp, len);
	if (r) {
		int32_t l = my_memcmp(dat, tmp, len);
		if (l == len) {
			r = true;
			printf("____ sf_check success!!!!\n");
		}
		else {
			r = false;
			printf("____ sf_check failed, l: %d, len: %d\n", l, len);
			sf_save("./uboot_dump.bin", tmp, len);
			//sf_save("../uboot_file.bin", dat, len);
		}
	}
	else {
		printf("____ sf_read failed!!!!\n");
	}

	free(tmp);

	return r;
}


bool sf_prog(uint32_t addr, int8_t *filename)
{
	FILE *file;
	bool r = true;
	size_t flen, rlen;
	uint8_t *buffer = NULL;

	printf("____ fopen %s\n", filename);
	
	file = fopen(filename, "rb");
	if (!file) {
		printf("____ uboot open failed, %d\n", errno);
		return false;
	}

	printf("____ fseek %s\n", filename);
	fseek(file, 0, SEEK_END);
	flen = ftell(file);
	if (flen <= 0) {
		printf("____ uboot length error\n");
		r = false;
		goto failed;

	}
	
	printf("____ malloc %s\n", filename);
	buffer = (uint8_t *)malloc(flen);
	if (!buffer) {
		printf("____ buffer malloc failed\n");
		r = false;
		goto failed;
	}
	
	printf("____ fread %s\n", filename);
	fseek(file, 0, SEEK_SET);
	rlen = fread(buffer, 1, flen, file);
	if (rlen != flen) {
		printf("____ fread failed, flen: %zd, rlen: %zd\n", flen, rlen);
		r = false;
		goto failed;

	}
	
	printf("____ sf_write %s\n", filename);
	r = sf_write(addr, buffer, (int32_t)flen);
	if (r) {
		printf("____ sf_check %s\n", filename);
		r = sf_check(addr, buffer, (int32_t)flen);
	}

failed:
	free(buffer);
	fclose(file);
	
	return r;
}