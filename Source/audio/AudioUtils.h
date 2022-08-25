#pragma once
#include "../arch/Smooth.h"
#include "../param/Param.h"
#include "../arch/State.h"
#include "../arch/Conversion.h"

namespace audio
{
    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = 3.14159265359f;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiHalfInv = 1.f / PiHalf;

    using AudioBuffer = juce::AudioBuffer<float>;
    using SIMD = juce::FloatVectorOperations;
    using MIDIBuffer = juce::MidiBuffer;
    using String = juce::String;
    using MIDIIt = juce::MidiBufferIterator;
    using MIDIRef = MIDIIt::reference;
    using Decibels = juce::Decibels;
    using ScopedNoDenormals = juce::ScopedNoDenormals;
    using MidiMessage = juce::MidiMessage;

    using Smooth = smooth::Smooth<float>;
    using PID = param::PID;
    using Params = param::Params;
    using Param = param::Param;
    using State = sta::State;
}