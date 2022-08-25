#pragma once
#include "AudioUtils.h"
#include "WHead.h"
#include <array>

namespace audio
{
	class DryWetMix
	{
		struct LatencyCompensation
		{
			LatencyCompensation();

			/*blockSize, latency*/
			void prepare(int, int);

			/*dry,inputSamples,numChannels,numSamples*/
			void operator()(float**, float**, int, int) noexcept;

		protected:
			AudioBuffer ring;
			WHead wHead;
			int latency;
		};

	public:
		DryWetMix();

		/* sampleRate, blockSize, latency */
		void prepare(float, int, int);

		/* samples, numChannels, numSamples, gainInP, mixP, gainP, polarityP, unityGainP */
		void saveDry(
			float**, int, int
#if PPDHasGainIn
			, float**
#endif
#if PPDHasPolarity
			, bool
#endif
#if PPDHasUnityGain && PPDHasGainIn
			, bool
#endif
		) noexcept;

		/*samples, numChannels, numSamples*/
		void processBypass(float**, int, int) noexcept;

		/*samples, numChannels, numSamples, gainOutP */
		void processGainOut(float**, int, int, float**) noexcept;
		
		/*samples, numChannels, numSamples, mixP */
		void processMix(float**, int, int, float**) noexcept;

	protected:
		LatencyCompensation latencyCompensation;

#if PPDHasGainIn
		std::array<Smooth, 2> gainInSmooth;
		AudioBuffer gainInBuffer;
#if PPDHasUnityGain
		AudioBuffer unityGainBuffer;
#endif
#endif
		std::array<Smooth, 2> mixSmooth, gainOutSmooth;
		AudioBuffer gainOutBuffer;

		AudioBuffer dryBuf;
	};
}