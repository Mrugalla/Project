#pragma once
#include "Button.h"

namespace gui
{
	struct PatchBrowser :
		public Comp
	{
		PatchBrowser(Utils& u) :
			Comp(u, "", CursorType::Default),
			closeButton(u, "Click here to close the browser")
		{
			layout.init(
				{ 1, 2, 2, 8, 21, 2, 2, 1 },
				{ 1, 2, 2, 8, 21, 1 }
			);

			makeTextButton(closeButton, "X", false);
			closeButton.getLabel().mode = Label::Mode::TextToLabelBounds;
			closeButton.getLabel().textCID = ColourID::Abort;
			closeButton.onClick.push_back([&]()
			{
				setVisible(false);
			});

			addAndMakeVisible(closeButton);

			setOpaque(true);
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xff000000));

			g.setColour(Colour(0x44ffffff));
			//layout.paint(g);

			layout.label(g, "v", 2, 1, 1, 1, true);
			layout.label(g, "current preset name", 3, 1, 2, 1, false);
			layout.label(g, "<", 5, 1, 1, 1, true);
			layout.label(g, ">", 6, 1, 1, 1, true);

			layout.label(g, "tags/filters/search bar etc", 1, 2, 3, 2, false);
			layout.label(g, "bar with sortables", 4, 2, 2, 1, false);
			layout.label(g, "list of\npatches", 4, 3, 2, 2, false);
			layout.label(g, "/\n/\n/\n/\n/\n", 6, 2, 1, 3, false);

			layout.label(g, "inspector\nview", 1, 4, 3, 1, false);
		}

		void resized() override
		{
			layout.resized();

			layout.place(closeButton, 1, 1, 1, 1, true);
		}

		String getCurrentPatchName()
		{
			return "dummy preset name";
		}

	protected:
		Button closeButton;
	};

	struct ButtonPatchBrowser :
		public Button
	{
		ButtonPatchBrowser(Utils& u, PatchBrowser& _browser) :
			Button(u, "Click here to open the patch browser."),
			browser(_browser)
		{
			makeTextButton(*this, browser.getCurrentPatchName(), false);
			onClick.push_back([&]()
			{
				const auto e = browser.isVisible();
				if(e)
					browser.setVisible(false);
				else
				{


					browser.setVisible(true);
				}
			});
		}

	protected:
		PatchBrowser& browser;
	};
}