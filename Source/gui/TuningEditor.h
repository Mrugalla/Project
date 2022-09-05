#pragma once
#include "Knob.h"

namespace gui
{
	struct TuningEditorButton :
		public Button
	{
		TuningEditorButton(Utils& u, CompWidgetable* tu) :
			Button(u, "This button opens the tuning editor, where you can manipulate the way notes are interpreted.")
		{
			makeSymbolButton(*this, ButtonSymbol::TuningFork);

			toggleState = 0;

			onClick.push_back([tu](Button& btn)
			{
				if (btn.toggleState == 0)
				{
					// open tuning editor
					tu->initWidget(.2f, false);
					btn.toggleState = 1;
				}
				else
				{
					// close tuning editor
					tu->initWidget(.1f, true);
					btn.toggleState = 0;
				}
			});
		}
	};

	struct XenWheel :
		public Knob
	{
		XenWheel(Utils& u) :
			Knob(u)
		{
			makeParameter(*this, PID::Xen, "Xen");
		}
	};

	struct TuningEditor :
		public CompWidgetable
	{
		TuningEditor(Utils& u) :
			CompWidgetable(u, "", CursorType::Default),
			xen(u)
		{
			addAndMakeVisible(xen);

			layout.init
			(
				{ 1, 8, 5, 1 },
				{ 1, 3, 1, 5, 5, 1, 5, 5, 1, 3, 1 }
			);
		}

		void paint(Graphics& g)
		{
			g.fillAll(Colours::c(ColourID::Bg).withAlpha(.98f));

			g.setColour(Colours::c(ColourID::Txt));
			layout.label(g, "tuning editor", 1, 1, 2, 1, false);
			//layout.label(g, "xen\nwheel", 1, 3, 1, 5, true);
			
			layout.label(g, "base note: 69", 2, 3, 1, 1, false);
			layout.label(g, "master tune: 440", 2, 4, 1, 1, false);
			
			layout.label(g, "pitch class: 'd#'", 2, 6, 1, 1, false);
			layout.label(g, "temperament: .1", 2, 7, 1, 1, false);

			layout.label(g, "keyboard component", 1, 9, 2, 1, false);
		}

		void resized()
		{
			layout.resized();

			layout.place(xen, 1, 3, 1, 5, true);
		}

	protected:
		XenWheel xen;

	};
}