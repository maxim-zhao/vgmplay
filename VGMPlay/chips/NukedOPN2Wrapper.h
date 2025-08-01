#pragma once
#include <inttypes.h>

#include "mamedef.h"

// C functions to hide the CPP
void* NukedOPN2Wrapper_new();
void NukedOPN2Wrapper_delete(void* wrapper);
void NukedOPN2Wrapper_reset(void* chip);
void NukedOPN2Wrapper_set_mute_mask(void* chip, uint32_t mask);
void NukedOPN2Wrapper_write(void* chip, uint32_t offset, uint8_t data);
void NukedOPN2Wrapper_stream_update(void* chip, stream_sample_t **outputs, int samples);

