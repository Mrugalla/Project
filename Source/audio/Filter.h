#pragma once
#include "../arch/Conversion.h"
#include <cmath>

namespace audio
{
	struct FilterBandpass
	{
		/* startVal */
		FilterBandpass(float startVal = 0.f) :
			a0(0.f),
			a1(0.f),
			a2(0.f),
			b1(0.f),
			b2(0.f),
			x1(0.f),
			x2(0.f),
			y1(startVal),
			y2(startVal)
		{
			
		}

		/* frequency fc [0, .5[, bandwidth bw [0, .5[ */
		void setFc(float fc, float bw) noexcept
		{
			auto R = 1.f - 3.f * bw;
			auto K = (1.f - 2.f * R * std::cos(Tau * fc) + R * R) / (2.f - 2.f * std::cos(Tau * fc));

			a0 = 1.f - K;
			a1 = 2.f * (K - R) * std::cos(Tau * fc);
			a2 = R * R - K;
			b1 = 2.f * R * std::cos(Tau * fc);
			b2 = -R * R;
		}
		
		float processSample(float x0) noexcept
		{
			y1 =
				x0 * a0 +
				x1 * a1 +
				x2 * a2 +
				y1 * b1 +
				y2 * b2;

			x2 = x1;
			x1 = x0;
			y2 = y1;
			
			return y1;
		}

		float a0, a1, a2, b1, b2;
		float     x1, x2, y1, y2;
	};
}