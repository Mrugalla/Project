#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include <cmath>

namespace audio
{
	struct NullNoiseSynth :
		public juce::Timer
	{
		using Host = juce::PluginHostType;
		using File = juce::File;
		using UniqueStream = std::unique_ptr<juce::FileInputStream>;

		NullNoiseSynth() :
			Timer(),
			inputStream(File(Host::getHostPath()).createInputStream()),
			noise(),
			readHead(0)
		{
			noise.resize(2205, 0.f);

			startTimerHz(24);
		}

		void operator()(float** samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				
				for (auto s = 0; s < numSamples; ++s)
				{
					smpls[s] = noise[readHead];
					readHead = (readHead + 1) % noise.size();
				}
			}
		}

		UniqueStream inputStream;
		std::vector<float> noise;
		int readHead;

		void timerCallback() override
		{
			auto& stream = *inputStream.get();

			for (auto n = 0; n < noise.size(); ++n)
			{
				if (stream.isExhausted())
					stream.setPosition(stream.getTotalLength() - stream.getPosition());

				auto smpl = stream.readFloatBigEndian();
				if (std::isnan(smpl) || std::isinf(smpl))
					smpl = 0.f;
				else
					smpl = std::fmod(smpl, 2.f) - 1.f;

				noise[n] = smpl;
			}
		}
	};
}

/*
this synth makes crappy noise from data that is used in a wrong way.
it's a fun side project. contributions are welcome
*/