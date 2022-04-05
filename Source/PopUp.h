#pragma once
#include "Button.h"
#include "GUIParams.h"

namespace gui
{
	template<size_t NumEntries>
	class PopUp :
		public CompWidgetable
	{
		Notify makeNotify(PopUp& popUp)
		{
			return [&pop = popUp](EvtType type, const void* stuff)
			{
				if (type == EvtType::ClickedEmpty || type == EvtType::ParametrDragged)
				{
					pop.setVisible(false);
				}
				else if (type == EvtType::ParametrRightClicked)
				{
					pop.setVisible(false);

					const auto& parametr = *static_cast<const Parametr*>(stuff);
					auto& utils = pop.getUtils();
					auto& pluginTop = utils.pluginTop;
					
					pop.setButton("Randomize", "Randomize this parameter value.", [param = utils.getParam(parametr.getPID())]()
					{
						juce::Random rand;
						param->setValueWithGesture(rand.nextFloat());
					}, 0);
					pop.setButton("Rand Relative", "Randomize this parameter value relative to its current value.", [param = utils.getParam(parametr.getPID())]()
					{
						juce::Random rand;
						auto val = param->getValue();
						val += .1f * (rand.nextFloat() - .5f);
						param->setValueWithGesture(juce::jlimit(0.f, 1.f, val));
					}, 1);
					pop.setButton("Load Default", "Resets this parameter value to its default value.", [param = utils.getParam(parametr.getPID())]()
					{
						const auto val = param->getDefaultValue();
						param->setValueWithGesture(val);
					}, 2);
					pop.setButton("Save Default", "Saves this parameter value as its default one.", [param = utils.getParam(parametr.getPID())]()
					{
						// yet to implement..
					}, 3);
					pop.setButton("Lock / Unlock", "Parameter values are locked into place, even when changing presets.", [param = utils.getParam(parametr.getPID())]()
					{
						param->switchLock();
					}, 4);
					pop.setButton("MIDI Learn", "Click here to assign this parameter to a hardware control.", [param = utils.getParam(parametr.getPID())]()
					{
						// yet to implement..
					}, 5);
					
					const auto screenPos = (parametr.getScreenPosition() + Point(parametr.getWidth() / 2, parametr.getHeight() / 2) - pluginTop.getScreenPosition()).toFloat();
					pop.defineBounds(
						BoundsF(screenPos.x, screenPos.y, 1.f, 1.f),
						BoundsF(screenPos.x, screenPos.y, 150.f, pluginTop.getHeight() * .5f)
					);
					pop.initWidget(.15f, false);
					pop.setVisible(true);
				}
			};
		}

	public:
		PopUp(Utils& u) :
			CompWidgetable(u, "", makeNotify(*this)),
			buttons(),
			origin(0.f, 0.f),
			bounds(0.f, 0.f, 0.f, 0.f)
		{
			buttons.reserve(NumEntries);
			for (auto i = 0; i < NumEntries; ++i)
				buttons.emplace_back(std::make_unique<Button>(utils, ""));
			
			for (auto& btn : buttons)
				addAndMakeVisible(*btn);
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc();
			g.setColour(Colours::c(ColourID::Darken));
			g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(thicc), thicc);
		}

		void setButton(String&& _name, String&& _tooltip, Button::OnClick&& _onClick, int i)
		{
			auto& btn = *buttons[i];

			btn.setTooltip(std::move(_tooltip));
			btn.onClick.clear();
			btn.onClick.push_back(_onClick);
			makeTextButton(btn, std::move(_name), true, true);
		}

		std::vector<std::unique_ptr<Button>> buttons;
	protected:
		PointF origin;
		BoundsF bounds;

		void resized() override
		{
			distributeVertically(*this, buttons);
		}
	};
}