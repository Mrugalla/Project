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

	void DryWetMix::LatencyCompensation::operator()(float** dry, float** inputSamples,
		int numChannels, int numSamples) noexcept
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

#if PPDHasGainIn
		gainInSmooth{1.f, 1.f},
		gainInBuffer(),
#if PPDHasUnityGain
		unityGainBuffer(),
#endif
#endif
		mixSmooth{1.f, 1.f}, gainOutSmooth{1.f, 1.f},
		gainOutBuffer(),

		dryBuf()
	{}

	void DryWetMix::prepare(float sampleRate, int blockSize, int latency)
	{
		latencyCompensation.prepare(blockSize, latency);

		for(auto& smooth: mixSmooth)
			smooth.makeFromDecayInMs(13.f, sampleRate);
		for (auto& smooth : mixSmooth)
			smooth.makeFromDecayInMs(13.f, sampleRate);
		for (auto& smooth : gainOutSmooth)
			smooth.makeFromDecayInMs(13.f, sampleRate);
		gainOutBuffer.setSize(2, blockSize, false, false, false);
#if PPDHasGainIn
		for (auto& smooth : gainInSmooth)
			smooth.makeFromDecayInMs(13.f, sampleRate);
		gainInBuffer.setSize(2, blockSize, false, false, false);
#if PPDHasUnityGain
		unityGainBuffer.setSize(2, blockSize, false, false, false);
#endif
#endif
		dryBuf.setSize(2, blockSize, false, true, false);
	}

	void DryWetMix::saveDry(float** samples, int numChannels, int numSamples
#if PPDHasGainIn
		, float** gainInP
#endif
#if PPDHasPolarity
		, bool polarityP
#endif
#if PPDHasUnityGain && PPDHasGainIn
		, bool unityGainP
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
#if PPDHasGainIn
		auto gainInSmoothBuf = gainInBuffer.getArrayOfWritePointers();
#if PPDHasUnityGain
		auto unityGainBufs = unityGainBuffer.getArrayOfWritePointers();
#endif
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto& smooth = gainInSmooth[ch];
			auto gainInBuf = gainInP[ch];
			
			auto gisb = gainInSmoothBuf[ch];
			smooth(gisb, gainInBuf, numSamples);

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto gainInDb = gisb[s];
				const auto gainIn = Decibels::decibelsToGain(gainInDb);
#if PPDHasUnityGain
				gainInBuf[s] = -gainInDb;
#endif
				smpls[s] *= gainIn;
			}
		}
#if PPDHasUnityGain
		
		if (!unityGainP)
			for(auto ch = 0; ch < numChannels; ++ch)
				SIMD::clear(unityGainBufs[ch], numSamples);
		else
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto unityGainBuf = unityGainBufs[ch];
				const auto gainInBuf = gainInP[ch];
				
				SIMD::copy(unityGainBuf, gainInBuf, numSamples);
			}
#endif
#endif
#if PPDHasPolarity
		if (polarityP)
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				SIMD::multiply(smpls, -1.f, numSamples);
			}
#endif
	}

	void DryWetMix::processBypass(float** samples, int numChannels, int numSamples) noexcept
	{
		latencyCompensation(
			dryBuf.getArrayOfWritePointers(),
			samples,
			numChannels,
			numSamples
		);

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			const auto dry = dryBuf.getReadPointer(ch);

			auto smpls = samples[ch];

			SIMD::copy(smpls, dry, numSamples);
		}
	}

	void DryWetMix::processGainOut(float** samples, int numChannels, int numSamples,
		float** gainOutP) noexcept
	{
#if PPDHasUnityGain
		auto unityGainBufs = unityGainBuffer.getArrayOfReadPointers();
#endif
		auto gainOutSmoothBuf = gainOutBuffer.getArrayOfWritePointers();
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto& smooth = gainOutSmooth[ch];
			auto gainOutBuf = gainOutP[ch];
#if PPDHasUnityGain
			const auto unityGainBuf = unityGainBufs[ch];
#endif
			auto gosb = gainOutSmoothBuf[ch];
			smooth(gosb, gainOutBuf, numSamples);
			
			for (auto s = 0; s < numSamples; ++s)
			{
				auto gainOutDb = gosb[s];
#if PPDHasUnityGain
				const auto unityGain = unityGainBuf[s];
				gainOutDb += unityGain;
#endif
				const auto gainOut = Decibels::decibelsToGain(gainOutDb);
				
				smpls[s] *= gainOut;
			}
		}
	}
	
	void DryWetMix::processMix(float** samples, int numChannels, int numSamples,
		float** mixP) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			const auto dry = dryBuf.getReadPointer(ch);
			
			auto& smooth = mixSmooth[ch];
			auto mix = mixP[ch];

			smooth(mix, numSamples);
			
			auto smpls = samples[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				auto d = dry[s];
				auto w = smpls[s];
				
				smpls[s] = d + mix[s] * (w - d);
			}
		}
	}
}