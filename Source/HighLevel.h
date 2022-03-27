#pragma once
#include "Comp.h"
#include "Button.h"
#include "Knob.h"
#include "Dial.h"

#include <array>
#include "config.h"

namespace gui
{
	struct HighLevel :
		public Comp
	{
		HighLevel(Utils& u) :
			Comp(u, "", CursorType::Default),
			pluginTitle(u, JucePlugin_Name),
			macro(u, "Macro", PID::Macro),
#if PPDHasGainIn
			gainIn(u, "Gain >", PID::GainIn),
			meterIn(gainIn, u.getMeter(0)),
#endif
			gainOut(u, "< Gain", PID::Gain),
			meterOut(gainOut, u.getMeter(PPDHasGainIn ? 1 : 0)),
			mix(u, "Mix", PID::Mix),
#if PPDHasUnityGain
			unityGain(u, param::toTooltip(PID::UnityGain)),
#endif
#if PPDHasHQ
			hq(u, param::toTooltip(PID::HQ)),
#endif
			stereoConfig(u, param::toTooltip(PID::StereoConfig)),
			power(u, param::toTooltip(PID::Power)),
			polarity(u, param::toTooltip(PID::Polarity)),
			patchSelect{
				Button(u, param::toTooltip(PID::PatchSelect)),
				Button(u, param::toTooltip(PID::PatchSelect))
			},
			patchMode(u, param::toTooltip(PID::PatchMode))
		{
			layout.init(
				{ 5, 30, 30, 30, 5, 5 },
				{ 5, 20, 30, 20, 30, 5, 15, 5, 15, 5, 15 }
			);

			pluginTitle.bgCID = ColourID::Transp;
			pluginTitle.outlineCID = ColourID::Transp;
			pluginTitle.font = getFontNEL();

			addAndMakeVisible(pluginTitle);
			addAndMakeVisible(macro);
#if PPDHasGainIn
			addAndMakeVisible(gainIn);
#endif
			addAndMakeVisible(gainOut);
			addAndMakeVisible(mix);
#if PPDHasUnityGain
			makeParameterSwitchButton(unityGain, PID::UnityGain, ButtonSymbol::UnityGain);
			addAndMakeVisible(unityGain);
#endif
#if PPDHasHQ
			makeParameterSwitchButton(hq, PID::HQ, "HQ");
			addAndMakeVisible(hq);
#endif
			makeParameterSwitchButton(stereoConfig, PID::StereoConfig, ButtonSymbol::StereoConfig);
			addAndMakeVisible(stereoConfig);

			makeParameterSwitchButton(power, PID::Power, ButtonSymbol::Power);
			addAndMakeVisible(power);

			makeParameterSwitchButton(polarity, PID::Polarity, ButtonSymbol::Polarity);
			addAndMakeVisible(polarity);

			makeParameterButtonsGroup(patchSelect, PID::PatchSelect, "AB");
			for (auto& ab: patchSelect)
				addAndMakeVisible(ab);

			makeParameterSwitchButton(patchMode, PID::PatchMode, ButtonSymbol::PatchMode);
			addAndMakeVisible(patchMode);

			setInterceptsMouseClicks(false, true);
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::c(ColourID::Hover));
			layout.label(g, "delta", 3.f, 1.25f, .5f, .75f, true);
			//layout.label(g, "mode", 1.5f, 4.25f, .5f, .5f, true);
			layout.label(g, "<", 1.25f, 6, .25f, 1, false);
			layout.label(g, "preset browser", 1.5f, 6, 1.5f, 1, false);
			layout.label(g, ">", 3.f, 6, .25f, 1, false);
			layout.label(g, "save", 3.25f, 6, .5f, 1, false);
			layout.label(g, "optns", 1.f, 8, 1.f, 1, true);
			layout.label(g, "<\n<\n<", 5, 0, 1, 11, false);
			
			const auto thicc = utils.thicc();
			auto thiccI = static_cast<int>(thicc) / 2;
			if (thiccI == 0)
				thiccI = 1;
			g.setColour(Colours::c(ColourID::Txt));
			{
				const auto y = static_cast<int>(layout.getY(4.5f));
				const auto left = layout.getX(1.75f);
				const auto right = layout.getX(2.125f);
				for (auto i = -thiccI; i < thiccI; ++i)
					g.drawHorizontalLine(y + i, left, right);
			}
#if PPDHasUnityGain
			{
				const auto y = static_cast<int>(layout.getY(3.5f));
				{
					const auto left = layout.getX(2.f);
					const auto right = layout.getX(2.125f);
					for (auto i = -thiccI; i < thiccI; ++i)
						g.drawHorizontalLine(y + i, left, right);
				}
				{
					const auto right = layout.getX(3.f);
					const auto left = layout.getX(2.875f);
					for (auto i = -thiccI; i < thiccI; ++i)
						g.drawHorizontalLine(y + i, left, right);
				}
			}
#endif
		}

		void resized() override
		{
			layout.resized();

			layout.place(pluginTitle, 1.25f, 10, 2.5f, 1, false);

			layout.place(macro, 2, 4, 1, 1, true);
#if PPDHasGainIn
			layout.place(gainIn, 1, 3, 1, 1, true);
#endif
			layout.place(gainOut, 3, 3, 1, 1, true);
			layout.place(mix, 2, 2, 1, 1, true);
#if PPDHasUnityGain
			layout.place(unityGain, 2.25f, 3.25f, .5f, .5f, true);
#endif
#if PPDHasHQ
			layout.place(hq, 2.f, 8, 1.f, 1, true);
#endif
			layout.place(stereoConfig, 3.f, 8, 1.f, 1, true);
			layout.place(power, 2.25f, 1.f, .5f, .75f, true);
			layout.place(polarity, 1.5f, 1.25f, .5f, .75f, true);
			layout.place(patchSelect[0], 1.75f, 4.5f, .5f, .5f, true);
			layout.place(patchSelect[1], 2.75f, 4.5f, .5f, .5f, true);
			layout.place(patchMode, 1.25f, 4.25f, .5f, .5f, true);
		}

	protected:
		Label pluginTitle;
		Knob macro;
#if PPDHasGainIn
		Knob gainIn;
		KnobMeter meterIn;
#endif
		Knob gainOut;
		KnobMeter meterOut;
		Knob mix;
#if PPDHasUnityGain
		Button unityGain;
#endif
#if PPDHasHQ
		Button hq;
#endif
		Button stereoConfig;
		Button power;
		Button polarity;
		std::array<Button, 2> patchSelect;
		Button patchMode;
	};
}


#include "configEnd.h"