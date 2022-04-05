#pragma once

#include "juce_core/juce_core.h"

namespace makeRange
{
	using Range = juce::NormalisableRange<float>;

	inline Range biased(float start, float end, float bias/*[-1, 1]*/) noexcept
	{
		// https://www.desmos.com/calculator/ps8q8gftcr
		const auto a = bias * .5f + .5f;
		const auto a2 = 2.f * a;
		const auto aM = 1.f - a;
		
		const auto r = end - start;
		const auto aR = r * a;
		if (bias != 0.f)
			return
		{
				start, end,
				[a2, aM, aR](float min, float, float x)
				{
					return min + aR * x / (aM - x + a2 * x);
				},
				[a2, aM, aR](float min, float, float x)
				{
					return aM * (x - min) / (a2 * min + aR - a2 * x - min + x);
				},
				nullptr
		};
		else return { start, end };
	}

	inline Range toggle() noexcept
	{
		return { 0.f, 1.f, 1.f };
	}

	inline Range stepped(float start, float end, float steps = 1.f) noexcept
	{
		return
		{
				start, end,
				[range = end - start](float min, float, float normalized)
				{
					return min + normalized * range;
				},
				[rangeInv = 1.f / (end - start)](float min, float, float denormalized)
				{
					return (denormalized - min) * rangeInv;
				},
				[steps, stepsInv = 1.f / steps](float, float, float val)
				{
					return std::rint(val * stepsInv) * steps;
				}
		};
	}

	// advanced one(s):

	inline Range withCentre(float start, float end, float centre) noexcept
	{
		static constexpr float Thresh = .0001f;

		auto b = 0.f;
		auto bInc = 1.f;
		auto range = biased(start, end, b);
		auto nVal = range.convertFrom0to1(.5f);
		auto dist = nVal - centre;
		auto dif = std::abs(dist);
		if (dif < Thresh)
			return range;
		do
		{
			bInc *= .5f;
			b += dist > 0.f ? -bInc : bInc;
			
			range = biased(start, end, b);
			nVal = range.convertFrom0to1(.5f);
			dist = nVal - centre;
			dif = std::abs(dist);

		} while (dif > Thresh);

		return range;
	}
}