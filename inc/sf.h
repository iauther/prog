#ifndef __SF_H__
#define __SF_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

bool sf_init();

bool sf_free();

bool sf_erase();

bool sf_write(uint32_t addr, uint8_t *data, int32_t len);

bool sf_read(uint32_t addr, uint8_t *data, int32_t len);

bool sf_save(int8_t *name, uint8_t *buf, int32_t len);

bool sf_check(uint32_t addr, uint8_t *dat, int32_t len);

bool sf_prog(uint32_t addr, int8_t *filename);

#endif

