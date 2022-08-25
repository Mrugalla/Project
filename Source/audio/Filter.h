#pragma once
#include <cmath>

namespace audio
{
	// This is a an infite impulse response (IIR) lowpass filter.
	// it doesn't work at all tho lol
	struct IIR
	{
		static constexpr double Pi = 3.14159265358979323846;
		static constexpr double Tau = 2. * Pi;
		
		IIR() :
			sampleRateInv(1.),
			a0(1.), a1(0.), a2(0.), b1(0.), b2(0.),
					x1(0.), x2(0.), y1(0.), y2(0.)
		{
		}

		void prepare(double sampleRate)
		{
			sampleRateInv = 1. / sampleRate;
		}

		double operator()(double x) noexcept
		{
			double y =
				a0 * x +
				a1 * x1 +
				a2 * x2 -
				b1 * y1 -
				b2 * y2;
			
			x2 = x1;
			x1 = x;
			
			y2 = y1;
			y1 = y;
			
			return y;
		}
		
		// updates the filter coefficients of this IIR-based biquad lowpass filter.
		void updateCoefficients(double frequencyHz, double q) noexcept
		{
			const auto fc = frequencyHz * sampleRateInv; // [0,.5]
			const auto w = Tau * fc; // [0, pi]
			const auto cosw = std::cos(w);
			const auto sinw = std::sin(w);

			const auto alpha = sinw / (2. * q);
			a0 = 1. / (1. + alpha);
			a1 = -2. * cosw * a0;
			a2 = (1. - alpha) * a0;
			
			b1 = -2. * cosw * a0;
			b2 = (1. - alpha) * a0;
		}

	protected:
		double sampleRateInv;
		double a0, a1, a2, b1, b2;
		double	   x1, x2, y1, y2;
	};
}