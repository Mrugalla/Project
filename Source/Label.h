#pragma once
#include "Comp.h"
#include "Shared.h"

namespace gui
{
	struct Label :
		public Comp
	{
		enum class Mode { WindowToTextBounds, TextToLabelBounds, NumModes };

		static constexpr float MinFontHeight = 7.f;

		Label(Utils& u, String&& _name, Notify&& _notify = [](EvtType, const void*){}) :
			Comp(u, "", std::move(_notify), gui::CursorType::Default),
			group(),
			textCID(ColourID::Txt),
			just(Just::centred),
			font(getFontNEL()),
			mode(Mode::WindowToTextBounds),
			text(_name)
		{
			font.setHeight(MinFontHeight);
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

		bool empty() const noexcept { return text.isEmpty(); }

		std::vector<Label*> group;
		ColourID textCID;
		Just just;
		Font font;
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
			float nHeight = MinFontHeight;

			if (mode == Mode::WindowToTextBounds)
			{
				auto val = utils.fontHeight();
				nHeight = std::max(nHeight, val);
			}
			if (mode == Mode::TextToLabelBounds)
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
			if (font != getFontNEL())
				nHeight += 2.5f;
			font.setHeight(nHeight);
		}
	};
}