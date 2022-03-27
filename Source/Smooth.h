#pragma once
#include <cmath>

namespace smooth
{
	template<typename Float>
	struct Smooth
	{
		static constexpr Float Pi = static_cast<Float>(3.14159265359);
		static constexpr Float Tau = Pi * static_cast<Float>(2);

		void makeFromDecayInSamples(Float d) noexcept
		{
			static constexpr auto e = static_cast<Float>(2.71828182846);
			//const auto x = std::pow(e, static_cast<Float>(-1) / d);
			const auto x = std::exp(static_cast<Float>(-1) / d);
			setX(x);
		}
		void makeFromDecayInSecs(Float d, Float Fs) noexcept
		{
			makeFromDecayInSamples(d * Fs);
		}
		void makeFromDecayInFc(Float fc) noexcept
		{
			static constexpr auto e = static_cast<Float>(2.71828182846);
			//const auto x = std::pow(e, -Tau * fc);
			const auto x = std::exp(-Tau * fc);
			setX(x);
		}
		void makeFromDecayInHz(Float d, Float Fs) noexcept
		{
			makeFromDecayInFc(d / Fs);
		}
		void makeFromDecayInMs(Float d, Float Fs) noexcept
		{
			makeFromDecayInSamples(d * Fs * static_cast<Float>(.001));
		}

		Smooth(const Float _startVal = static_cast<Float>(0)) :
			a0(static_cast<Float>(1)),
			b1(static_cast<Float>(0)),
			y1(_startVal),
			eps(static_cast<Float>(0)),
			startVal(_startVal)
		{}

		void reset()
		{
			a0 = static_cast<Float>(1);
			b1 = static_cast<Float>(0);
			y1 = startVal;
			eps = static_cast<Float>(0);
		}

		void operator()(Float* buffer, Float val, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				buffer[s] = processSample(val);
		}
		void operator()(Float* buffer, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				buffer[s] = processSample(buffer[s]);
		}
		Float operator()(Float sample) noexcept
		{
			return processSample(sample);
		}

	protected:
		Float a0, b1, y1, eps, startVal;

		Float processSample(Float x0) noexcept
		{
			y1 = x0 * a0 + y1 * b1;
			return y1;
		}

		void setX(Float x) noexcept
		{
			a0 = static_cast<Float>(1) - x;
			b1 = x;
			eps = a0 * static_cast<Float>(1.5);
		}
	};
}