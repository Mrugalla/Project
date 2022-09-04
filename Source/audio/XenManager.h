#pragma once
#include "../arch/Conversion.h"
#include <array>
#include <atomic>

namespace audio
{
	struct XenManager
	{
		XenManager() :
			xen(12.f),
			masterTune(440.f),
			temperaments()
		{
			for (auto& t : temperaments)
				t = 0.f;
		}
		
		void operator()(float _xen, float _masterTune) noexcept
		{
			xen = _xen;
			masterTune = _masterTune;
		}

		template<typename Float>
		Float noteToFreqHz(Float note) const noexcept
		{
			return noteInFreqHz(note, static_cast<Float>(69), xen, masterTune);
		}

		template<typename Float>
		Float noteToFreqHzWithWrap(Float note, Float lowestFreq, Float highestFreq = static_cast<Float>(22000)) const noexcept
		{
			auto freq = noteToFreqHz(note);
			while (freq < lowestFreq)
				freq *= 2.f;
			while (freq >= 22000.f)
				freq *= .5f;
			return freq;
		}

	protected:
		float xen, masterTune;
		std::array<std::atomic<float>, PPD_MaxXen> temperaments;
	};
}