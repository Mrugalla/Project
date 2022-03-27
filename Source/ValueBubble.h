#pragma once
#include "Comp.h"
#include "Label.h"
#include "Knob.h"
#include "config.h"

namespace gui
{
	class ValueBubble : 
		public Label,
		public Timer
	{
		static constexpr int FreezeLength = static_cast<int>(PPDFPSValueBubble * PPDValueBubbleVisibleTimeMs * .001f);
		static constexpr float FreezeLenInv = 1.f / static_cast<float>(FreezeLength);

		Notify makeNotify(ValueBubble& valueBubble)
		{
			return [&vb = valueBubble](EvtType t, const void* stuff)
			{
				if (t == EvtType::ComponentAdded)
				{
					vb.setAlwaysOnTop(true);
					vb.repaint();
				}
				else if (t == EvtType::KnobHovered)
				{
					const auto knob = reinterpret_cast<const Parametr*>(stuff);
					vb.update(knob->getPID());
				}
				else if (t == EvtType::KnobValueChanged)
				{
					const auto knob = reinterpret_cast<const Parametr*>(stuff);
					vb.update(knob->getPID());
				}
			};
		}
	public:
		ValueBubble(Utils& u) :
			Label(u, "", makeNotify(*this)),
			pID(PID::Power),
			freezeIdx(0)
		{
			font = getFontNEL();
			font.setHeightWithoutChangingWidth(font.getHeight() * 1.2f);
			outlineCID = ColourID::Transp;
			bgCID = ColourID::Darken;
			textCID = ColourID::Txt;
		}

		void update(PID _pID, const Comp* comp)
		{
			pID = _pID;
			const auto param = utils.getParam(pID);
			setText(param->getCurrentValueAsText());
			const auto centrePos = comp->getScreenPosition() + comp->getLocalBounds().getCentre() - utils.getScreenPosition();
			setCentrePosition(centrePos);
			freezeIdx = 0;
			setVisible(true);
			startTimerHz(static_cast<int>(PPDFPSValueBubble));
		}
		void update(PID _pID)
		{
			pID = _pID;
			const auto param = utils.getParam(pID);
			setText(param->getCurrentValueAsText());
			const auto centrePos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().toInt() - utils.getScreenPosition();
			setCentrePosition(centrePos);
			freezeIdx = 0;
			setVisible(true);
			startTimerHz(static_cast<int>(PPDFPSValueBubble));
		}

	protected:
		PID pID;
		int freezeIdx = 0;

		void timerCallback() override
		{
			++freezeIdx;
			if (freezeIdx >= FreezeLength)
			{
				setVisible(false);
				return;
			}
			const auto a = static_cast<float>(freezeIdx) * FreezeLenInv;
			setAlpha(1.f - a * a * a * a);
			repaint();
		}
	};
}

#include "configEnd.h"