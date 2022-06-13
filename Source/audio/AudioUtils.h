#pragma once
#include "../arch/Smooth.h"
#include "../param/Param.h"
#include "../arch/State.h"

namespace audio
{
    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = 3.14159265359f;

    using AudioBuffer = juce::AudioBuffer<float>;
    using SIMD = juce::FloatVectorOperations;
    using String = juce::String;

    using Smooth = smooth::Smooth<float>;
    using PID = param::PID;
    using Params = param::Params;
    using State = sta::State;

    template<typename Float>
    inline Float msInSamples(Float ms, Float Fs) noexcept
    {
        return ms * Fs * static_cast<Float>(.001);
    }
}