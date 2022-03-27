#include "Comp.h"

namespace gui
{
	struct Tooltip :
		public Comp
	{
		Tooltip(Utils& _utils, String&& _tooltip) :
			Comp(_utils, _tooltip, makeNotify(this), CursorType::Default),
			curTooltip(""),
			BuildDate(static_cast<String>(JucePlugin_Manufacturer) + " Plugins, v: " + static_cast<String>(__DATE__) + " " + static_cast<String>(__TIME__))
		{
			setBufferedToImage(true);
		}
		
		void updateTooltip(const String* t)
		{
			curTooltip = t == nullptr ? "" : *t;
			repaint();
		}

	protected:
		String curTooltip;
		
		void paint(Graphics& g) override
		{
			g.setColour(Colours::c(ColourID::Hover));
			g.drawFittedText(BuildDate, getLocalBounds(), Just::centredRight, 1);
			g.setColour(Colours::c(ColourID::Txt));
			g.drawFittedText(curTooltip, getLocalBounds(), Just::left, 1);
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