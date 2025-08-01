extern "C"
{
#include "YMFMWrapper.h"
}

#include "ymfm/src/ymfm_opl.h"
#include <queue>

class ymfm_ym2413: public ymfm::ym2413, public ymfm::ymfm_interface
{
	output_data output_;
public:
	ymfm_ym2413()
	: ym2413(*static_cast<ymfm_interface*>(this)),
	output_()
	{
		reset();
	}

	int32_t generate(const uint32_t mask, const uint32_t channel_mask)
	{
		// This is based on opll_base::generate()
		m_fm.clock(fm_engine::ALL_CHANNELS);  // NOLINT(clang-diagnostic-undefined-func-template)
		m_fm.output(output_.clear(), 5, 256, channel_mask);  // NOLINT(clang-diagnostic-undefined-func-template)

		// Now we need to extricate the output we really wanted...
		int32_t output = 0;
		if (mask & 0b00000111111111)
		{
			// Some melody was wanted
			output += output_.data[0];
		}
		if (mask & 0b11111000000000)
		{
			// Some rhythm was wanted
			output += output_.data[1];
		}

		return output * 128 / 9;
	}
};

class YMFMWrapper
{
	ymfm_ym2413 opll_{};
	uint32_t mute_mask_{0};
	uint32_t mask_for_ymfm_{0xffff};

public:
	void reset()
	{
		opll_.reset();
	}

	void stream_update(stream_sample_t** outputs, const int samples)
	{
		auto* pLeft = outputs[0];
		auto* pRight = outputs[1];

		for (int i = 0; i < samples; ++i, pLeft++, pRight++)
		{
			*pLeft = *pRight = opll_.generate(mute_mask_, mask_for_ymfm_);
		}
	}

	void write(const uint32_t address, const uint8_t data)
	{
		opll_.write(address, data);
	}

	void set_mute_mask(const uint32_t mute_mask)
	{
		mute_mask_ = mute_mask;

		// ymfm emulates the 9 channels, and for each emits melody + rhythm.
		// Unfortunately that means you can't mute specific rhythm channels.
		// We merge them into the 9 bits it understands, and also invert so 1 = on.
		mask_for_ymfm_ = ~mute_mask;
		mask_for_ymfm_ =
			(mask_for_ymfm_ & 0b111111111) // Tone channels
			| (((mask_for_ymfm_ >>  9) & 1) << 6)   // Bass drum is channel 7 = bit 6
			| (((mask_for_ymfm_ >> 10) & 1) << 7)   // Snare drum is channel 8 = bit 7
			| (((mask_for_ymfm_ >> 11) & 1) << 7)   // Tom-tom is channel 8 = bit 7
			| (((mask_for_ymfm_ >> 12) & 1) << 8)   // Cymbal is channel 9 = bit 8
			| (((mask_for_ymfm_ >> 13) & 1) << 8);  // Hi-hat is channel 9 = bit 8
	}
};

extern "C"
{
	void* YMFMWrapper_new(uint8_t mode)
	{
		return new YMFMWrapper();
	}

	void YMFMWrapper_delete(void* chip)
	{
		delete static_cast<YMFMWrapper*>(chip);
	}

	void YMFMWrapper_reset(void* chip, uint8_t mode)
	{
		static_cast<YMFMWrapper*>(chip)->reset();
	}

	void YMFMWrapper_set_mute_mask(void* chip, uint32_t mask)
	{
		static_cast<YMFMWrapper*>(chip)->set_mute_mask(mask);
	}

	void YMFMWrapper_write(void* chip, uint32_t offset, uint8_t data)
	{
		static_cast<YMFMWrapper*>(chip)->write(offset, data);
	}

	void YMFMWrapper_stream_update(void* chip, stream_sample_t** outputs, int samples)
	{
		static_cast<YMFMWrapper*>(chip)->stream_update(outputs, samples);
	}
}
