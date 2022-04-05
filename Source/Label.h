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

		ColourID textCID;
		Just just;
		Font font;
	protected:
		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc();
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);
			const auto y = bounds.getY();
			const auto h = bounds.getHeight();
			const auto fontH = font.getHeight();
			const auto nH = y + (h - fontH) * .5f;
			g.setColour(Colours::c(textCID));
			g.setFont(font);
			g.drawFittedText(getName(), bounds.withY(nH).toNearestInt(), just, 1);
		}

		void resized() override
		{
			updateTextBounds();
		}

	private:
		void updateTextBounds(float minFontHeight = 6.f)
		{
			const auto thicc = utils.thicc();

			const auto& text = getName();
			float maxStrWidth = 0.f;
			{
				auto sIdx = 0;
				for (auto i = 1; i < text.length(); ++i)
				{
					if (text[i] == '\n')
					{
						const auto lineWidth = font.getStringWidthFloat(text.substring(sIdx, i));
						if (maxStrWidth < lineWidth)
							maxStrWidth = lineWidth;
						++i;
						sIdx = i;
					}
				}
				const auto lineWidth = font.getStringWidthFloat(text.substring(sIdx));
				if (maxStrWidth < lineWidth)
					maxStrWidth = lineWidth;
			}

			const auto width = static_cast<float>(getWidth());
			const auto ratio = width / maxStrWidth;

			auto fontHeight = font.getHeight();
			fontHeight *= ratio;

			const auto height = static_cast<float>(getHeight());
			if (fontHeight > height)
				fontHeight = height;

			const auto nHeight = fontHeight - thicc;

			font.setHeight(nHeight < minFontHeight ? minFontHeight : nHeight);
		}
	};
}