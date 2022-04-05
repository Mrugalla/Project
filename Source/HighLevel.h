#pragma once
#include "Comp.h"
#include "Button.h"
#include "Knob.h"
#include "Dial.h"
#include "Menu.h"
#include "LowLevel.h"

#include <array>
#include "config.h"

namespace gui
{
	struct HighLevel :
		public Comp
	{
		HighLevel(Utils& u, LowLevel* _lowLevel) :
			Comp(u, "", CursorType::Default),
			pluginTitle(u, JucePlugin_Name),
			macro(u, "Macro", PID::Macro, false),
#if PPDHasGainIn
			gainIn(u, "Gain >", PID::GainIn, false),
			meterIn(gainIn, u.getMeter(0)),
#endif
			gainOut(u, "< Gain", PID::Gain, false),
			meterOut(gainOut, u.getMeter(PPDHasGainIn ? 1 : 0)),
			mix(u, "Mix", PID::Mix, false),
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
			patchMode(u, param::toTooltip(PID::PatchMode)),

			lowLevel(_lowLevel),

			menu(nullptr),
			menuButton(u, "Click here to open or close the panel with the advanced settings.")
		{
			layout.init(
				{ 5, 30, 30, 30, 5, 5 },
				{ 5, 15, 30, 30, 40, 15, 5, 15, 5, 10 }
			);

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

			makeParameterButtonsGroup(patchSelect, PID::PatchSelect, "AB", true);
			for(auto i = 0; i < 2; ++i)
			{
				auto& ab = patchSelect[i];
				auto& label = ab.getLabel();
				label.textCID = ColourID::Mod;
				addAndMakeVisible(ab);
			}

			makeParameterSwitchButton(patchMode, PID::PatchMode, ButtonSymbol::PatchMode);
			addAndMakeVisible(patchMode);

			makeSymbolButton(menuButton, ButtonSymbol::Settings);
			menuButton.toggleState = 0;
			menuButton.onClick.push_back([this]()
			{
				auto& btn = menuButton;

				auto& ts = btn.toggleState;
				ts = ts == 0 ? 1 : 0;
				repaintWithChildren(&btn);

				if (ts == 1)
				{
					auto& pluginTop = utils.pluginTop;

					const auto xml = loadXML(BinaryData::menu_xml, BinaryData::menu_xmlSize);
					if (xml == nullptr)
						return;
					const auto vt = ValueTree::fromXml(*xml);
					if (!vt.isValid())
						return;

					menu.reset(new Menu(utils, vt));
					pluginTop.addAndMakeVisible(*menu);

					const auto bounds1 = lowLevel->getBounds().toFloat();
					const auto bounds0 = bounds1.withLeft(static_cast<float>(pluginTop.getRight()));

					menu->defineBounds(bounds0, bounds1);
					menu->initWidget(.1f, false);
				}
				else
				{
					menu.reset(nullptr);
				}
			});
			addAndMakeVisible(menuButton);

			setInterceptsMouseClicks(false, true);
		}

		void paint(Graphics& g) override
		{
			Comp::paint(g);
			layout.paint(g);

			g.setColour(Colours::c(ColourID::Hover));
			layout.label(g, "delta", 3.f, 1.25f, .5f, .75f, true);
			layout.label(g, "<", 1.25f, 5, .25f, 1, false);
			layout.label(g, "preset browser", 1.5f, 5, 1.5f, 1, false);
			layout.label(g, ">", 3.f, 5, .25f, 1, false);
			layout.label(g, "save", 3.25f, 5, .5f, 1, false);
			layout.label(g, "<\n<\n<", 5, 0, 1, 10, false);
			
			const auto thicc = utils.thicc();
			auto thiccI = static_cast<int>(thicc) / 2;
			if (thiccI == 0)
				thiccI = 1;
			g.setColour(Colours::c(ColourID::Txt));
			{
				const auto y = static_cast<int>(layout.getY(4.5f));
				const auto left = layout.getX(1.5f);
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
			layout.place(hq, 2.f, 7, 1.f, 1, true);
#endif
			layout.place(stereoConfig, 3.f, 7, 1.f, 1, true);
			layout.place(power, 2.25f, 1.f, .5f, .75f, true);
			layout.place(polarity, 1.5f, 1.25f, .5f, .75f, true);
			layout.place(patchSelect[0], 1.5f + .125f, 4.5f, .5f, .5f, true);
			layout.place(patchSelect[1], 2.75f + .125f, 4.5f, .5f, .5f, true);
			layout.place(patchMode, 1.125f, 4.25f, .5f, .5f, true);

			layout.place(menuButton, 1.f, 7, 1.f, 1, true);

			if (menu != nullptr)
			{
				menu->defineBounds(menu->getBounds().toFloat(), lowLevel->getBounds().toFloat());
				menu->initWidget(.3f, false);
			}
				
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

		LowLevel* lowLevel;
		std::unique_ptr<Menu> menu;
		Button menuButton;
	};
}


#include "configEnd.h"