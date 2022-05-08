#pragma once
#include "Comp.h"
#include "Shared.h"

namespace gui
{
	struct Label :
		public Comp
	{
		enum class Mode { None, WindowToTextBounds, TextToLabelBounds, NumModes };

		Label(Utils& u, const String& _text, Notify&& _notify = [](EvtType, const void*) {}) :
			Comp(u, "", std::move(_notify), gui::CursorType::Default),
			group(),
			textCID(ColourID::Txt),
			just(Just::centred),
			font(getFontDosisExtraBold()),
			minFontHeight(12.f),
			mode(Mode::WindowToTextBounds),
			text(_text)
		{
			font.setHeight(minFontHeight);
			setInterceptsMouseClicks(false, false);
		}

		void setText(const String& txt)
		{
			if (txt == text)
				return;

			text = txt;

			if (empty() || getWidth() == 0 || getHeight() == 0)
				return;
			
			updateTextBounds();
		}

		const String& getText() const noexcept
		{
			return text;
		}

		void setMinFontHeight(float h)
		{
			minFontHeight = h;
			updateTextBounds();
		}

		bool empty() const noexcept { return text.isEmpty(); }

		std::vector<Label*> group;
		ColourID textCID;
		Just just;
		Font font;
		float minFontHeight;
		Mode mode;
	protected:
		String text;

		void paint(Graphics& g) override
		{
			const auto bounds = getLocalBounds().toFloat();
			g.setColour(Colours::c(textCID));
			g.setFont(font);
			g.drawFittedText(text, bounds.toNearestInt(), just, 1);
		}

		void resized() override
		{
			updateTextBounds();
		}

	private:
		void updateTextBounds()
		{
			float nHeight = minFontHeight;

			if (mode == Mode::WindowToTextBounds)
			{
				auto val = utils.fontHeight();
				nHeight = std::max(nHeight, val);
			}

			else if (mode == Mode::TextToLabelBounds)
			{
				const auto thicc = utils.thicc();

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

				nHeight = std::max(fontHeight - thicc, nHeight);
			}

			else if (mode == Mode::None)
			{
				font.setHeight(nHeight);
			}

			font.setHeight(nHeight);
		}
	};
}