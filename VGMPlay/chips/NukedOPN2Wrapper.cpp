extern "C"
{
#include "NukedOPN2Wrapper.h"
#include "Nuked-OPN2/ym3438.h"
}

#include <queue>

class NukedOPN2Wrapper
{
	ym3438_t chip_;
	uint32_t mute_mask_{0};
	std::queue<std::pair<uint32_t, uint8_t>> buffered_writes_;
public:
	NukedOPN2Wrapper()
	{
		reset();
	}

	void reset()
	{
		OPN2_Reset(&chip_);
	}

	void stream_update(stream_sample_t** outputs, const int samples)
	{
		stream_sample_t* pLeft = outputs[0];
		stream_sample_t* pRight = outputs[1];
		Bit16s mixed_output[2];

		for (int i = 0; i < samples; ++i, ++pLeft, ++pRight)
		{
			int32_t left = 0;
			int32_t right = 0;

			// We clock the chip 24 times per sample...
			for (int j = 0; j < 24; ++j)
			{
				// If we have any buffered writes, we can emit them while clocking,
				// but not too fast...
				if (!buffered_writes_.empty() && !chip_.write_busy)
				{
					const auto& write = buffered_writes_.front();
					OPN2_Write(&chip_, write.first, write.second);
					buffered_writes_.pop();
				}

				// Figure out the channel about to be produced
				uint32_t channel_mask;
				switch (chip_.cycles >> 2)
				{
				default: // is impossible
				case 0: channel_mask = 1 << 1; break;
				case 1: channel_mask = 1 << (5 + chip_.dacen); break;
				case 2: channel_mask = 1 << 3; break;
				case 3: channel_mask = 1 << 0; break;
				case 4: channel_mask = 1 << 4; break;
				case 5: channel_mask = 1 << 2; break;
				}

				// Get the sample from the chip
				OPN2_Clock(&chip_, mixed_output);

				// Then we figure out which channel that was and only use it if not muted
				if ((mute_mask_ & channel_mask) == 0)
				{
					left += mixed_output[0];
					right += mixed_output[1];
				}
			}

			// Multiply output by 11 to be roughly comparable to other cores(?)
			*pLeft = left * 11;
			*pRight = right * 11;
		}
	}

	void buffer_write(uint32_t address, uint8_t data)
	{
		buffered_writes_.emplace(address, data);
	}

	void set_mute_mask(const uint32_t mute_mask)
	{
		mute_mask_ = mute_mask;
	}
};

extern "C"
{
	void* NukedOPN2Wrapper_new()
	{
		return new NukedOPN2Wrapper();
	}

	void NukedOPN2Wrapper_delete(void* chip)
	{
		delete static_cast<NukedOPN2Wrapper*>(chip);
	}

	void NukedOPN2Wrapper_reset(void* chip)
	{
		static_cast<NukedOPN2Wrapper*>(chip)->reset();
	}

	void NukedOPN2Wrapper_set_mute_mask(void* chip, uint32_t mask)
	{
		static_cast<NukedOPN2Wrapper*>(chip)->set_mute_mask(mask);
	}

	void NukedOPN2Wrapper_write(void* chip, uint32_t offset, uint8_t data)
	{
		static_cast<NukedOPN2Wrapper*>(chip)->buffer_write(offset, data);
	}

	void NukedOPN2Wrapper_stream_update(void* chip, stream_sample_t** outputs, int samples)
	{
		static_cast<NukedOPN2Wrapper*>(chip)->stream_update(outputs, samples);
	}
}
