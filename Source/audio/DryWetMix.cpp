#include "DryWetMix.h"

namespace audio
{
	// DryWetMix::LatencyCompensation

	DryWetMix::LatencyCompensation::LatencyCompensation() :
		ring(),
		wHead(),
		latency(0)
	{}

	void DryWetMix::LatencyCompensation::prepare(int blockSize, int _latency)
	{
		latency = _latency;
		if (latency != 0)
		{
			ring.setSize(2, latency, false, true, false);
			wHead.prepare(blockSize, latency);
		}
		else
		{
			ring.setSize(0, 0);
			wHead.prepare(0, 0);
		}
	}

	void DryWetMix::LatencyCompensation::operator()(float** dry, float** inputSamples, int numChannels, int numSamples) noexcept
	{
		if (latency != 0)
		{
			wHead(numSamples);

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto smpls = inputSamples[ch];

				auto rng = ring.getWritePointer(ch);
				auto dr = dry[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = (w + 1) % latency;

					rng[w] = smpls[s];
					dr[s] = rng[r];
				}
			}
		}
		else
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::copy(dry[ch], inputSamples[ch], numSamples);
	}

	// DryWetMix

	DryWetMix::DryWetMix() :
		latencyCompensation(),

		buffers(),

#if PPDHasGainIn
		gainInSmooth(0.f),
#endif
		mixSmooth(1.f),
		gainOutSmooth(1.f),

		dryBuf()
	{}

	void DryWetMix::prepare(float sampleRate, int blockSize, int latency)
	{
		latencyCompensation.prepare(blockSize, latency);
		dryBuf.setSize(2, blockSize, false, true, false);

		buffers.setSize(NumBufs, blockSize, false, true, false);

#if PPDHasGainIn
		gainInSmooth.makeFromDecayInMs(20.f, sampleRate);
#endif
		mixSmooth.makeFromDecayInMs(20.f, sampleRate);
		gainOutSmooth.makeFromDecayInMs(20.f, sampleRate);
	}

	void DryWetMix::saveDry(float** samples, int numChannels, int numSamples,
#if PPDHasGainIn
		float gainInP,
#endif
		float mixP, float gainOutP
#if PPDHasPolarity
		, float polarityP
#endif
#if PPDHasUnityGain && PPDHasGainIn
		, float unityGainP
#endif
	) noexcept
	{
		latencyCompensation
		(
			dryBuf.getArrayOfWritePointers(),
			samples,
			numChannels,
			numSamples
		);

		auto bufs = buffers.getArrayOfWritePointers();

#if PPDHasGainIn
		{
			auto gainInBuf = bufs[GainIn];
			gainInSmooth(gainInBuf, decibelToGain(gainInP), numSamples);
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samples[ch], gainInBuf, numSamples);

#if PPDHasUnityGain
			if (unityGainP)
				for (auto s = 0; s < numSamples; ++s)
				{
					auto d1 = gainToDecibel(gainInBuf[s]);
					auto d2 = -d1;
					auto g2 = decibelToGain(d2);
					
					gainInBuf[s] = g2;
				}
#endif
		}
#endif
		if (polarityP)
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samples[ch], -1.f, numSamples);

		auto gainOutBuf = bufs[GainOut];
		gainOutSmooth(gainOutBuf, decibelToGain(gainOutP), numSamples);

		auto mixBuf = bufs[Mix];
		mixSmooth(mixBuf, mixP, numSamples);
	}

	void DryWetMix::processBypass(float** samples, int numChannels, int numSamples) noexcept
	{
		latencyCompensation
		(
			dryBuf.getArrayOfWritePointers(),
			samples,
			numChannels,
			numSamples
		);

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			const auto dry = dryBuf.getReadPointer(ch);

			auto smpls = samples[ch];

			for (auto s = 0; s < numSamples; ++s)
				smpls[s] = dry[s];
		}
	}

	void DryWetMix::processOutGain(float** samples, int numChannels, int numSamples) const noexcept
	{
		const auto bufs = buffers.getArrayOfReadPointers();

#if PPDHasUnityGain && PPDHasGainIn
		auto gainInBuf = bufs[GainIn];
#endif
		const auto gainOutBuf = bufs[GainOut];
		
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			
#if PPDHasUnityGain && PPDHasGainIn
			SIMD::multiply(smpls, gainInBuf, numSamples);
#endif
			SIMD::multiply(smpls, gainOutBuf, numSamples);
		}
			
	}

	void DryWetMix::processMix(float** samples, int numChannels, int numSamples) const noexcept
	{
		const auto dryBf = dryBuf.getArrayOfReadPointers();
		
		const auto bufs = buffers.getArrayOfReadPointers();
		const auto mix = bufs[Mix];

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			const auto dry = dryBf[ch];
			auto smpls = samples[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto d = dry[s];
				const auto m = mix[s];

				const auto smpl = smpls[s];

				smpls[s] = d + m * (smpl - d);
			}
		}
	}
}