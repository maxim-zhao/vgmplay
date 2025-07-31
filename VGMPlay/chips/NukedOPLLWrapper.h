#pragma once
#include <inttypes.h>

#include "mamedef.h"

// C functions to hide the CPP
void* NukedOPLLWrapper_new(uint8_t mode);
void NukedOPLLWrapper_delete(void* wrapper);
void NukedOPLLWrapper_reset(void* chip, uint8_t mode);
void NukedOPLLWrapper_set_mute_mask(void* chip, uint32_t mask);
void NukedOPLLWrapper_write(void* chip, uint32_t offset, uint8_t data);
void NukedOPLLWrapper_stream_update(void* chip, stream_sample_t **outputs, int samples);

