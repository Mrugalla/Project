#pragma once
#include "Comp.h"
#include "Shared.h"

namespace gui
{
	struct Label :
		public Comp
	{
		Label(Utils& u, String&& _name, Notify&& _notify = [](EvtType, const void*){}) :
			Comp(u, "", std::move(_notify), gui::CursorType::Default),
			bgCID(ColourID::Bg),
			outlineCID(ColourID::Txt),
			textCID(ColourID::Txt),
			just(Just::centred),
			font(getFontNEL())
		{
			setInterceptsMouseClicks(false, false);
			setText(_name);
		}

		void setText(const String& txt)
		{
			if (txt == getName())
				return;

			setName(txt);

			if (txt.isEmpty() || getWidth() == 0 || getHeight() == 0)
				return;
			
			updateTextBounds();
		}

		bool empty() const noexcept { return getName().isEmpty(); }

		ColourID bgCID, outlineCID, textCID;
		Just just;
		Font font;
	protected:
		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc();
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);
			g.setColour(Colours::c(bgCID));
			g.fillRoundedRectangle(bounds, thicc);
			g.setColour(Colours::c(outlineCID));
			g.drawRoundedRectangle(bounds, thicc, thicc);
			g.setColour(Colours::c(textCID));
			g.setFont(font);
			g.drawFittedText(getName(), bounds.toNearestInt(), just, 1);
		}

		void resized() override
		{
			updateTextBounds();
		}

	private:
		void updateTextBounds()
		{
			const auto thicc = utils.thicc();

			const auto strWidth = font.getStringWidthFloat(getName());
			const auto width = static_cast<float>(getWidth());
			const auto ratio = width / strWidth;

			auto fontHeight = font.getHeight();
			fontHeight *= ratio;

			const auto height = static_cast<float>(getHeight());
			if (fontHeight > height)
				fontHeight = height;

			font.setHeight(fontHeight - thicc);
		}
	};
}