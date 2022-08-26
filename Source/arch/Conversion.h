#pragma once

namespace audio
{
    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = 3.14159265359f;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiHalfInv = 1.f / PiHalf;
	
    template<typename Float>
    inline Float secsInSamples(Float secs, Float Fs) noexcept
    {
        return secs * Fs;
    }

    template<typename Float>
    inline Float msInSamples(Float ms, Float Fs) noexcept
    {
        return secsInSamples(ms * static_cast<Float>(.001), Fs);
    }

    template<typename Float>
    inline Float freqHzInSamples(Float hz, Float Fs) noexcept
    {
        return Fs / hz;
    }

    template<typename Float>
    inline float getRMS(const Float* ar, const int size) noexcept
    {
        auto rms = static_cast<Float>(0);
        for (auto i = 0; i < size; ++i)
            rms += ar[i] * ar[i];
        rms /= static_cast<Float>(size);
        return std::sqrt(rms);
    }

    template<typename Float>
    inline Float noteInFreqHz(Float note, Float rootNote = static_cast<Float>(69), Float xen = static_cast<Float>(12), Float masterTune = static_cast<Float>(440)) noexcept
    {
        return std::exp2((note - rootNote) / xen) * masterTune;
    }

	template<typename Float>
    inline Float noteInFreqHz2(Float note, Float rootNote = static_cast<Float>(69), Float masterTune = static_cast<Float>(440)) noexcept
    {
        return std::exp2((note - rootNote) * static_cast<Float>(.08333333333)) * masterTune;
    }

    template<typename Float>
    inline Float freqHzInNote(Float freqHz, Float rootNote = static_cast<Float>(69), Float xen = static_cast<Float>(12), Float masterTune = static_cast<Float>(440)) noexcept
    {
        return std::log2(freqHz / masterTune) * xen + rootNote;
    }

    template<typename Float>
    inline Float freqHzInNote(Float freqHz, Float xen = static_cast<Float>(12), Float rootNote = static_cast<Float>(69)) noexcept
    {
        return std::log2(freqHz * static_cast<Float>(.00227272727)) * xen + rootNote;
    }

	template<typename Float>
    inline Float gainToDecibel(Float gain) noexcept
    {
		return std::log10(gain) * static_cast<Float>(20);
    }

	template<typename Float>
	inline Float decibelToGain(Float db) noexcept
	{
		return std::pow(static_cast<Float>(10), db * static_cast<Float>(.05));
	}
}