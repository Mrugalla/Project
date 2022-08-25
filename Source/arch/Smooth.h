#pragma once

namespace smooth
{
	// a block-based parameter smoother.
	template<typename Float>
	struct Block
	{
		/* startVal */
		Block(float = 0.f);

		/* bufferOut, bufferIn, numSamples */
		void operator()(Float*, Float*, int) noexcept;

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;

		/* buffer, numSamples */
		void operator()(Float*, int) noexcept;

	protected:
		Float curVal;
	};
	
	template<typename Float>
	struct Lowpass
	{
		static constexpr Float Pi = static_cast<Float>(3.14159265359);
		static constexpr Float Tau = Pi * static_cast<Float>(2);

		void makeFromDecayInSamples(Float) noexcept;
		void makeFromDecayInSecs(Float, Float/*Fs*/) noexcept;
		void makeFromDecayInFc(Float) noexcept;
		void makeFromDecayInHz(Float, Float/*Fs*/) noexcept;
		void makeFromDecayInMs(Float, Float/*Fs*/) noexcept;

		void copyCutoffFrom(const Lowpass<Float>&) noexcept;

		Lowpass(const Float /*_startVal*/ = static_cast<Float>(0));

		void reset();

		void operator()(Float*, Float/*val*/, int/*numSamples*/) noexcept;
		void operator()(Float*, int/*numSamples*/) noexcept;
		Float operator()(Float) noexcept;

	protected:
		Float a0, b1, y1, eps, startVal;

		Float processSample(Float) noexcept;

		void setX(Float) noexcept;
	};

	template<typename Float>
	struct Smooth
	{
		/*smoothLenMs, Fs*/
		void makeFromDecayInMs(Float, Float);

		Smooth(float /*startVal*/ = 0.f);

		/* bufferOut, bufferIn, numSamples */
		void operator()(Float*, Float*, int) noexcept;

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;

		/* buffer, numSamples */
		void operator()(Float*, int) noexcept;

	protected:
		Block<Float> block;
		Lowpass<Float> lowpass;
	};
}