#pragma once
#include <cmath>

namespace audio
{
	inline void crush(float* samples, int numSamples, float gain,
		const float gainInv) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
			samples[s] = std::rint(samples[s] * gain) * gainInv;
	}

	inline void crush(float** samples, int numChannels, int numSamples, float gain) noexcept
	{
		const auto gainInv = 1.f / gain;
		for (auto ch = 0; ch < numChannels; ++ch)
			crush(samples[ch], numSamples, gain, gainInv);
	}

	inline void crush(float** samples, int numChannels, int numSamples,
		float** gain) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto g = gain[ch];

			SIMD::multiply(smpls, g, numSamples);
			
			for (auto s = 0; s < numSamples; ++s)
			{
				smpls[s] = std::rint(smpls[s]);
				g[s] = 1.f / g[s];
			}
			
			SIMD::multiply(smpls, g, numSamples);
		}
	}

	
}