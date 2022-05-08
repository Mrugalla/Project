#include "Label.h"

namespace gui
{
	struct Tooltip :
		public Comp
	{
		Tooltip(Utils& _utils, String&& _tooltip) :
			Comp(_utils, _tooltip, makeNotify(this), CursorType::Default),
			buildDateLabel(utils, static_cast<String>(JucePlugin_Manufacturer) + " Plugins, v: " + static_cast<String>(__DATE__) + " " + static_cast<String>(__TIME__)),
			tooltipLabel(utils, "")
		{
			buildDateLabel.textCID = ColourID::Hover;
			buildDateLabel.just = Just::centredRight;
			buildDateLabel.mode = Label::Mode::TextToLabelBounds;
			buildDateLabel.font = getFontDosisLight();
			tooltipLabel.textCID = ColourID::Txt;
			tooltipLabel.just = Just::centredLeft;
			tooltipLabel.mode = buildDateLabel.mode;
			tooltipLabel.font = buildDateLabel.font;

			addAndMakeVisible(buildDateLabel);
			addAndMakeVisible(tooltipLabel);
		}
		
		void updateTooltip(const String* t)
		{
			tooltipLabel.setText(t == nullptr ? "" : *t);
			tooltipLabel.repaint();
		}

	protected:
		Label buildDateLabel, tooltipLabel;
		
		void paint(Graphics&) override
		{
		}

		void resized() override
		{
			buildDateLabel.setBounds(getLocalBounds());
			tooltipLabel.setBounds(getLocalBounds());
		}

	private:
		Notify makeNotify(Tooltip* ttc)
		{
			return [ttc](const EvtType type, const void* stuff)
			{
				if (type == EvtType::TooltipUpdated)
				{
					const auto str = static_cast<const String*>(stuff);
					ttc->updateTooltip(str);
				}
			};
		}

		const String BuildDate;
	};
}