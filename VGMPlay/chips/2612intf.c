/***************************************************************************

  2612intf.c

  The YM2612 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include <stdlib.h>
#include "mamedef.h"
#include "fm.h"
#include "2612intf.h"

#include <stdio.h>
#ifdef ENABLE_ALL_CORES
#include "ym2612.h"
#include "NukedOPN2Wrapper.h"
#endif


#define EC_MAME		0x00	// YM2612 core from MAME (now fixed, so it isn't worse than Gens anymore)
#ifdef ENABLE_ALL_CORES
#define EC_GENS		0x01	// Gens YM2612 core from in_vgm
#define EC_NUKED	0x02	// Nuked OPN2 via wrapepr
#endif

typedef struct _ym2612_state ym2612_state;
struct _ym2612_state
{
	void *			chip;
	int			EMU_CORE;
	int* GensBuf[0x02];
	UINT8 ChipFlags;
};

void ym2612_update_request(void *param)
{
}

/***********************************************************/
/*    YM2612                                               */
/***********************************************************/

//static STREAM_UPDATE( ym2612_stream_update )
void ym2612_stream_update(void *_info, stream_sample_t **outputs, int samples)
{
	ym2612_state *info = _info;
	
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		ym2612_update_one(info->chip, outputs, samples);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_GENS:
		YM2612_ClearBuffer(info->GensBuf, samples);
		YM2612_Update(info->chip, info->GensBuf, samples);
		YM2612_DacAndTimers_Update(info->chip, info->GensBuf, samples);
		for (int i = 0x00; i < samples; i ++)
		{
			outputs[0x00][i] = info->GensBuf[0x00][i];
			outputs[0x01][i] = info->GensBuf[0x01][i];
		}
		break;
	case EC_NUKED:
		NukedOPN2Wrapper_stream_update(info->chip, outputs, samples);
		break;
#endif
	}
}

//static DEVICE_START( ym2612 )
int device_start_ym2612(void **_info, int EMU_CORE, int ChipFlags, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE, UINT8 * IsVGMInit)
{
	//static const ym2612_interface dummy = { 0 };
	//ym2612_state *info = get_safe_token(device);
	ym2612_state *info;
	int rate;

	info = (ym2612_state *) calloc(1, sizeof(ym2612_state));
	*_info = (void *) info;

	info->EMU_CORE = EMU_CORE;
	info->ChipFlags = ChipFlags;
	rate = clock/72;
	if (EMU_CORE == EC_MAME && ! (ChipFlags & 0x02))
		rate /= 2;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	//info->intf = device->static_config ? (const ym2612_interface *)device->static_config : &dummy;
	//info->intf = &dummy;
	//info->device = device;

	/* FM init */
	/* Timer Handler set */
	//info->timer[0] = timer_alloc(device->machine, timer_callback_2612_0, info);
	//info->timer[1] = timer_alloc(device->machine, timer_callback_2612_1, info);

	/* stream system initialize */
	//info->stream = stream_create(device,0,2,rate,info,ym2612_stream_update);

	/**** initialize YM2612 ****/
	switch(EMU_CORE)
	{
	case EC_MAME:
		//info->chip = ym2612_init(info,clock,rate,timer_handler,IRQHandler);
		info->chip = ym2612_init(info, clock, rate, NULL, NULL, IsVGMInit, ChipFlags);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_GENS:
		if (info->GensBuf[0x00] == NULL)
		{
			info->GensBuf[0x00] = malloc(sizeof(int) * 0x100);
			info->GensBuf[0x01] = info->GensBuf[0x00] + 0x80;
		}
		info->chip = YM2612_Init(clock, rate, 0x00);
		YM2612_SetMute(info->chip, 0x80);	// Disable SSG-EG
		YM2612_SetOptions(ChipFlags);
		break;
	case EC_NUKED:
		info->chip = NukedOPN2Wrapper_new();
		break;
#endif
	}

	return rate;
}


//static DEVICE_STOP( ym2612 )
void device_stop_ym2612(void *_info)
{
	//ym2612_state *info = get_safe_token(device);
	ym2612_state *info = (ym2612_state *)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		ym2612_shutdown(info->chip);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_GENS:
		YM2612_End(info->chip);
		if (info->GensBuf[0x00] != NULL)
		{
			free(info->GensBuf[0x00]);
			info->GensBuf[0x00] = NULL;
			info->GensBuf[0x01] = NULL;
		}
		break;
	case EC_NUKED:
		NukedOPN2Wrapper_delete(info->chip);
		break;
#endif
	}

	free(info);
}

//static DEVICE_RESET( ym2612 )
void device_reset_ym2612(void *_info)
{
	//ym2612_state *info = get_safe_token(device);
	ym2612_state *info = (ym2612_state *)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		ym2612_reset_chip(info->chip);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_GENS:
		YM2612_Reset(info->chip);
		break;
	case EC_NUKED:
		NukedOPN2Wrapper_reset(info->chip);
		break;
#endif
	}
}

void ym2612_w(void *_info, offs_t offset, UINT8 data)
{
	//ym2612_state *info = get_safe_token(device);
	ym2612_state *info = (ym2612_state *)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		ym2612_write(info->chip, offset & 3, data);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_GENS:
		YM2612_Write(info->chip, (unsigned char)(offset & 0x03), data);
		break;
	case EC_NUKED:
		NukedOPN2Wrapper_write(info->chip, offset & 3, data);
		break;
#endif
	}
}

void ym2612_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ym2612_state *info = (ym2612_state *)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		ym2612_set_mutemask(info->chip, MuteMask);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_GENS:
		YM2612_SetMute(info->chip, (int)MuteMask);
		break;
	case EC_NUKED:
		NukedOPN2Wrapper_set_mute_mask(info->chip, MuteMask);
		break;
#endif
	}
	
	return;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ym2612 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ym2612_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ym2612 );	break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ym2612 );	break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ym2612 );	break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YM2612");					break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");				break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}


DEVICE_GET_INFO( ym3438 )
{
	switch (state)
	{
		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YM3438");							break;

		default:										DEVICE_GET_INFO_CALL(ym2612);						break;
	}
}*/
