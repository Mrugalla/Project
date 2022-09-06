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
			baseNote(69.f),
			temperaments()
		{
			for (auto& t : temperaments)
				t = 0.f;
		}
		
		void operator()(float _xen, float _masterTune, float _baseNote) noexcept
		{
			xen = _xen;
			masterTune = _masterTune;
			baseNote = _baseNote;
		}

		template<typename Float>
		Float noteToFreqHz(Float note) const noexcept
		{
			return noteInFreqHz(note, baseNote, xen, masterTune);
		}

		template<typename Float>
		Float noteToFreqHzWithWrap(Float note, Float lowestFreq, Float highestFreq = static_cast<Float>(22000)) const noexcept
		{
			auto freq = noteToFreqHz(note);
			while (freq < lowestFreq)
				freq *= static_cast<Float>(2);
			while (freq >= highestFreq)
				freq *= static_cast<Float>(.5);
			return freq;
		}

		float getXen() const noexcept
		{
			return xen;
		}

	protected:
		float xen, masterTune, baseNote;
		std::array<std::atomic<float>, PPD_MaxXen> temperaments;
	};
}