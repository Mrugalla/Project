#pragma once
#include "WHead.h"
#include <array>
#include <vector>
#include <cmath>

namespace audio
{
	struct ImpulseResponse
	{
		ImpulseResponse(const std::vector<float>& _buf = { 1.f }) :
			buf(_buf),
			latency(static_cast<int>(size()) / 2)
		{
		}

		ImpulseResponse(const ImpulseResponse& other) :
			buf(other.buf),
			latency(other.latency)
		{
		}

		void operator=(const std::vector<float>& _buf)
		{
			buf = _buf;
			latency = static_cast<int>(_buf.size()) / 2;
		}

		float operator[](int i) const noexcept { return buf[i]; }
		const size_t size() const noexcept { return buf.size(); }

		const int getLatency() const noexcept { return latency; }

		const float* data() const noexcept { return buf.data(); }
	protected:
		std::vector<float> buf;
		int latency;
	};

	/*
	Nyquist == Fs / 2
	fc < Nyquist
	bw < Nyquist
	fc + bw < Nyquist
	*/
	inline std::vector<float> makeWindowedSinc(float Fs, float fc, float bw, bool upsampling)
	{
		const auto nyquist = Fs * .5f;
		if (fc > nyquist || bw > nyquist || fc + bw > nyquist)
			return {};

		fc /= Fs;
		bw /= Fs;

		int M = static_cast<int>(4.f / bw);
		if (M % 2 != 0)
			M += 1; // M is even number

		const auto Mf = static_cast<float>(M);
		const auto MHalf = Mf * .5f;
		const float MInv = 1.f / Mf;
		
		const int N = M + 1;

		const auto h = [&](float i)
		{ // sinc
			i -= MHalf;
			if (i != 0.f)
				return std::sin(Tau * fc * i) / i;
			return Tau * fc;
		};

		const auto w = [&, tau2 = Tau * 2.f](float i)
		{ // blackman window
			i *= MInv;
			return .42f - .5f * std::cos(Tau * i) + .08f * std::cos(tau2 * i);
		};

		std::vector<float> ir;
		ir.reserve(N);
		for (auto n = 0; n < N; ++n)
		{
			auto nF = static_cast<float>(n);
			ir.emplace_back(h(nF) * w(nF));
		}

		const auto targetGain = upsampling ? 2.f : 1.f;
		auto sum = 0.f; // normalize
		for (const auto n : ir)
			sum += n;
		const auto sumInv = targetGain / sum;
		for (auto& n : ir)
			n *= sumInv;

		return ir;
	}

	/*
	Nyquist == Fs / 2
	fc < Nyquist
	*/
	inline std::vector<float> makeWindowedSinc(float Fs, float fc, bool upsampling)
	{
		return makeWindowedSinc(Fs, fc, Fs * .25f - fc - 1.f, upsampling);
	}

	struct Convolver
	{
		Convolver(const ImpulseResponse& _ir, const WHead& _wHead) :
			ring(),
			ir(_ir),
			wHead(_wHead),
			irSize(0)
		{
		}

		void prepare()
		{
			irSize = static_cast<int>(ir.size());
			ring.setSize(2, irSize, false, true, false);
		}

		void processBlock(float** samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				auto rng = ring.getWritePointer(ch);

				processBlock(smpls, rng, numSamples);
			}
		}

	protected:
		AudioBuffer ring;
		const ImpulseResponse& ir;
		const WHead& wHead;
		int irSize;

	private:
		void processBlock(float* smpls, float* rng, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				smpls[s] = processSample(smpls[s], rng, wHead[s]);
		}

		float processSample(float smpl, float* rng, int w) noexcept
		{
			rng[w] = smpl;

			auto y = 0.f;
			auto r = w;

			for (auto i = 0; i < irSize; ++i)
			{
				y += rng[r] * ir[i];
				--r;
				if (r == -1)
					r = irSize - 1;
			}

			return y;
		}
	};

	inline void zeroStuff(float** samplesUp, const float** samplesIn, int numChannels, int numSamples1x) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto upBuf = samplesUp[ch];
			const auto inBuf = samplesIn[ch];

			for (auto s = 0; s < numSamples1x; ++s)
			{
				const auto s2 = s * 2;
				upBuf[s2] = inBuf[s];
				upBuf[s2 + 1] = 0.f;
			}
		}
	}

	inline void decimate(float** samplesOut, const float** samplesUp, int numChannels, int numSamples1x) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto outBuf = samplesOut[ch];
			const auto upBuf = samplesUp[ch];

			for (auto s = 0; s < numSamples1x; ++s)
				outBuf[s] = upBuf[s * 2];
		}
	}

	class Oversampler
	{
		static constexpr float CutoffFreq = 18000.f;
	public:
		Oversampler() :
			Fs(0.),
			blockSize(0),

			buffer(),

			irUp(),
			irDown(),
			wHead(),

			filterUp(irUp, wHead),
			filterDown(irDown, wHead),

			FsUp(0.),
			blockSizeUp(0),

			numSamples1x(0), numSamples2x(0),

			enabled(true)
		{
		}

		Oversampler(Oversampler& other) :
			Fs(other.Fs),
			blockSize(other.blockSize),
			buffer(other.buffer),
			irUp(other.irUp),
			irDown(other.irDown),
			wHead(other.wHead),
			filterUp(irUp, wHead),
			filterDown(irDown, wHead),
			FsUp(other.FsUp),
			blockSizeUp(other.blockSizeUp),
			numSamples1x(other.numSamples1x),
			numSamples2x(other.numSamples2x),
			enabled(other.enabled.load()),
			enbld(other.enbld)
		{}

		void prepare(const double sampleRate, const int _blockSize)
		{
			enbld = isEnabled();

			Fs = sampleRate;
			blockSize = _blockSize;

			if (enbld)
			{
				FsUp = Fs * 2.;
				blockSizeUp = blockSize * 2;

				irUp = makeWindowedSinc(static_cast<float>(FsUp), 19000.f, true);
				irDown = makeWindowedSinc(static_cast<float>(FsUp), 19000.f, false);

				filterUp.prepare();
				filterDown.prepare();

				buffer.setSize(2, blockSizeUp, false, true, false);
				wHead.prepare(blockSizeUp, static_cast<int>(irDown.size()));
			}
			else
			{
				FsUp = Fs;
				blockSizeUp = blockSize;
			}
		}

		AudioBuffer& upsample(AudioBuffer& inputBuffer) noexcept
		{
			enbld = isEnabled();
			if (enbld)
			{
				numSamples1x = inputBuffer.getNumSamples();
				numSamples2x = numSamples1x * 2;
				const auto numChannels = inputBuffer.getNumChannels();

				buffer.setSize(numChannels, numSamples2x, true, false, true);

				auto samplesUp = buffer.getArrayOfWritePointers();
				const auto samplesIn = inputBuffer.getArrayOfReadPointers();

				wHead(numSamples2x);

				zeroStuff(samplesUp, samplesIn, numChannels, numSamples1x);
				filterUp.processBlock(samplesUp, numChannels, numSamples2x);

				return buffer;
			}
			
			return inputBuffer;
		}

		void downsample(AudioBuffer& outputBuffer) noexcept
		{
			if (enbld)
			{
				auto samplesUp = buffer.getArrayOfWritePointers();
				auto samplesOut = outputBuffer.getArrayOfWritePointers();
				const auto samplesUpConst = buffer.getArrayOfReadPointers();
				const auto numChannels = outputBuffer.getNumChannels();

				// filter 2x + decimating
				filterDown.processBlock(samplesUp, numChannels, numSamples2x);
				decimate(samplesOut, samplesUpConst, numChannels, numSamples1x);
			}
		}

		const int getLatency() const noexcept
		{
			if(isEnabled())
				return (irUp.getLatency() + irDown.getLatency()) / 2;
			return 0;
		}
		double getFsUp() const noexcept { return FsUp; }
		int getBlockSizeUp() const noexcept { return blockSizeUp; }

		bool isEnabled() const noexcept { return enabled.load(); }

		/* only call this if processor is suspended! */
		void setEnabled(bool e) noexcept { enabled.store(e); }
	protected:
		double Fs;
		int blockSize;

		AudioBuffer buffer;

		ImpulseResponse irUp, irDown;
		WHead wHead;
		Convolver filterUp, filterDown;

		double FsUp;
		int blockSizeUp;

		int numSamples1x, numSamples2x;

		std::atomic<bool> enabled;
		bool enbld;
	};

}