extern "C"
{
#include "NukedOPLLWrapper.h"
#include "Nuked-OPLL/opll.h"
}

#include <queue>

class NukedOPLLWrapper
{
	opll_t opll_;
	uint32_t mute_mask_{0};
	std::queue<std::pair<uint32_t, uint8_t>> buffered_writes_;
	int clocks_until_next_write_{0};
public:
	explicit NukedOPLLWrapper(const int mode)
	{
		reset(mode);
	}

	void reset(const int mode)
	{
		OPLL_Reset(&opll_, mode ? opll_type_ds1001 : opll_type_ym2413);
	}

	void stream_update(stream_sample_t** outputs, const int samples)
	{
		stream_sample_t* pLeft = outputs[0];
		stream_sample_t* pRight = outputs[1];
		for (int i = 0; i < samples; ++i, ++pLeft, ++pRight)
		{
			int32_t out = 0;
			// Nuked OPLL emulates the YM2413's round-robin channel outputs,
			// but we want to pretend it outputs one (summed) sample every 18 calls.
			for (int cycle = 0; cycle < 18; ++cycle)
			{
				// If we have any buffered writes, we can emit them while clocking,
				// but not too fast...
				if (!buffered_writes_.empty() && clocks_until_next_write_ <= 0)
				{
					const auto& write = buffered_writes_.front();
					OPLL_Write(&opll_, write.first, write.second);
					if (write.first == 0)
					{
						// Address write: must wait 12 clocks
						clocks_until_next_write_ = 12;
					}
					else
					{
						// Data write: must wait 84 clocks
						clocks_until_next_write_ = 84;
					}

					buffered_writes_.pop();
				}

				// Get the sample from the chip
				int32_t buf[2];
				OPLL_Clock(&opll_, buf);

				if (clocks_until_next_write_ > 0)
				{
					--clocks_until_next_write_;
				}

				// Then sum it based on the muting. Outputs are (when indexed by opll_.cycles):
				//   Operator  Melody  Rhythm
			    //  0 mod7             Hi-hat #1
				//  1 mod8             Tom-tom #1
				//  2 car6     Tone 7  Bass drum #1
				//  3 car7     Tone 8  Snare drum #1
				//  4 car8     Tone 9  Top cymbal #1
				//  5 mod0             Hi-hat #2
				//  6 mod1             Tom-tom #2
				//  7 mod2             Bass drum #2
				//  8 car0     Tone 1
				//  9 car1     Tone 2
				// 10 car2     Tone 3
				// 11 mod3             Snare drum #2
				// 12 mod4             Top cymbal #2
				// 13 mod5
				// 14 car3     Tone 4
				// 15 car4     Tone 5
				// 16 car5     Tone 6
				// 17 mod6
				// We therefore collect each one in turn and add it to the output (if not muted).
				// The rhythm channel output twice, so we instead capture the first one and double it.
				switch (opll_.cycles)
				{
				case 0:
					if ((mute_mask_ & (1 << 13)) == 0) out += buf[1] * 2; // Rhythm 0 = HH = channel 13
					break;
				case 1:
					if ((mute_mask_ & (1 << 11)) == 0) out += buf[1] * 2; // Rhythm 1 = TOM = channel 11
					break;
				case 2:
					if ((mute_mask_ & (1 << 6)) == 0) out += buf[0]; // Tone 6
					if ((mute_mask_ & (1 << 9)) == 0) out += buf[1] * 2; // Rhythm 2 = BD = channel 9
					break;
				case 3:
					if ((mute_mask_ & (1 << 7)) == 0) out += buf[0]; // Tone 7
					if ((mute_mask_ & (1 << 10)) == 0) out += buf[1] * 2; // Rhythm 3 = SD = channel 10
					break;
				case 4:
					if ((mute_mask_ & (1 << 8)) == 0) out += buf[0]; // Tone 8
					if ((mute_mask_ & (1 << 12)) == 0) out += buf[1] * 2; // Rhythm 4 = CYM = channel 12
					break;
				case 8:
					if ((mute_mask_ & (1 << 0)) == 0) out += buf[0]; // Tone 0
					break;
				case 9:
					if ((mute_mask_ & (1 << 1)) == 0) out += buf[0]; // Tone 1
					break;
				case 10:
					if ((mute_mask_ & (1 << 2)) == 0) out += buf[0]; // Tone 2
					break;
				case 14:
					if ((mute_mask_ & (1 << 3)) == 0) out += buf[0]; // Tone 3
					break;
				case 15:
					if ((mute_mask_ & (1 << 4)) == 0) out += buf[0]; // Tone 4
					break;
				case 16:
					if ((mute_mask_ & (1 << 5)) == 0) out += buf[0]; // Tone 5
					break;
				default:
					break;
				}
			}
			// Multiply output by 8 to be roughly comparable to MAME and emu2413
			*pLeft = *pRight = out * 8;
		}
	}

	void buffer_write(offs_t address, UINT8 data)
	{
		buffered_writes_.emplace(address, data);
	}

	void setMuteMask(const uint32_t mute_mask)
	{
		mute_mask_ = mute_mask;
	}
};

extern "C"
{
	void* NukedOPLLWrapper_new(uint8_t mode)
	{
		return new NukedOPLLWrapper(mode);
	}

	void NukedOPLLWrapper_delete(void* chip)
	{
		delete static_cast<NukedOPLLWrapper*>(chip);
	}

	void NukedOPLLWrapper_reset(void* chip, uint8_t mode)
	{
		static_cast<NukedOPLLWrapper*>(chip)->reset(mode);
	}

	void NukedOPLLWrapper_set_mute_mask(void* chip, uint32_t mask)
	{
		static_cast<NukedOPLLWrapper*>(chip)->setMuteMask(mask);
	}

	void NukedOPLLWrapper_write(void* chip, uint32_t offset, uint8_t data)
	{
		static_cast<NukedOPLLWrapper*>(chip)->buffer_write(offset, data);
	}

	void NukedOPLLWrapper_stream_update(void* chip, stream_sample_t** outputs, int samples)
	{
		static_cast<NukedOPLLWrapper*>(chip)->stream_update(outputs, samples);
	}
}
