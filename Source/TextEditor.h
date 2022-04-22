#pragma once
#include "Label.h"

namespace gui
{
	struct TextEditor :
		public Comp
	{
		TextEditor(Utils& u, const String& _tooltip, const String& _emptyString = "enter value..") :
			Comp(u, _tooltip),
			label(u, ""),
			emptyString(_emptyString)
		{
			addAndMakeVisible(label);
		}

	protected:
		Label label;
		String emptyString;

		void paint(Graphics&) override { }

		void resized() override
		{
			label.setBounds(getLocalBounds());
		}
	};
}