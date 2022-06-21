#pragma once
#include "Comp.h"

namespace gui
{
	struct LogoComp :
		public Comp
	{
		LogoComp(Utils& u, std::unique_ptr<juce::Drawable>&& svg) :
			Comp(u, "logo tooltip", CursorType::Default),
			drawable(std::move(svg))
		{
			addAndMakeVisible(*drawable);
		}

		std::unique_ptr<juce::Drawable> drawable;

		void paint(Graphics&) override
		{

		}

		void resized() override
		{
			drawable->setBounds(getLocalBounds());
		}
	};
}
