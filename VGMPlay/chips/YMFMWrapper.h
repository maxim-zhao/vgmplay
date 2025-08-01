#pragma once
#include <inttypes.h>

#include "mamedef.h"

// C functions to hide the CPP
void* YMFMWrapper_new(uint8_t mode);
void YMFMWrapper_delete(void* wrapper);
void YMFMWrapper_reset(void* chip, uint8_t mode);
void YMFMWrapper_set_mute_mask(void* chip, uint32_t mask);
void YMFMWrapper_write(void* chip, uint32_t offset, uint8_t data);
void YMFMWrapper_stream_update(void* chip, stream_sample_t **outputs, int samples);

