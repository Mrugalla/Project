#pragma once
#include "Button.h"
#include "GUIParams.h"
#include "TextEditor.h"

namespace gui
{
	class PopUp :
		public CompWidgetable
	{
		Notify makeNotify(PopUp& popUp)
		{
			return [&pop = popUp](EvtType type, const void*)
			{
				if (type == EvtType::ClickedEmpty ||
					type == EvtType::ParametrDragged ||
					type == EvtType::EnterParameterValue)
				{
					pop.setVisible(false);
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
			
		}

		void init()
		{
			for (auto& b : buttons)
				addAndMakeVisible(*b);
		}

		void place(const Comp* comp)
		{
			auto& pluginTop = utils.pluginTop;

			const auto screenPos = (comp->getScreenPosition() + Point(comp->getWidth() / 2, comp->getHeight() / 2) - pluginTop.getScreenPosition()).toFloat();
			BoundsF dest(screenPos.x, screenPos.y, 150.f, pluginTop.getHeight() * .5f);
			if (dest.getBottom() > pluginTop.getBottom())
				dest.setY(pluginTop.getBottom() - dest.getHeight());
			if (dest.getRight() > pluginTop.getWidth())
				dest.setX(pluginTop.getWidth() - dest.getWidth());

			defineBounds(
				BoundsF(screenPos.x, screenPos.y, 1.f, 1.f),
				dest
			);
			initWidget(.15f, false);
			setVisible(true);
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc();
			g.setColour(Colours::c(ColourID::Darken));
			g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(thicc), thicc);
		}

		void addButton(String&& _name, String&& _tooltip)
		{
			buttons.emplace_back(std::make_unique<Button>(utils, std::move(_tooltip)));
			auto& btn = *buttons.back();
			makeTextButton(btn, std::move(_name), false);
		}
		void setButton(const Button::OnClick& _onClick, int idx)
		{
			auto& btn = *buttons[idx];
			btn.onClick.clear();
			btn.onClick.push_back(_onClick);
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

	class PopUpKnobs :
		public PopUp
	{
		Notify makeNotify2(PopUpKnobs& popUp)
		{
			return [&pop = popUp](EvtType type, const void* stuff)
			{
				if (type == EvtType::ParametrRightClicked)
				{
					const auto& parametr = *static_cast<const Parametr*>(stuff);
					auto& utils = pop.getUtils();
					
					pop.setButton([param = utils.getParam(parametr.getPID())]()
					{
						juce::Random rand;
						param->setValueWithGesture(rand.nextFloat());
					}, 0);
					pop.setButton([param = utils.getParam(parametr.getPID())]()
					{
						juce::Random rand;
						auto val = param->getValue();
						val += .05f * (rand.nextFloat() - .5f);
						param->setValueWithGesture(juce::jlimit(0.f, 1.f, val));
					}, 1);
					pop.setButton([param = utils.getParam(parametr.getPID())]()
					{
						const auto val = param->getDefaultValue();
						param->setValueWithGesture(val);
					}, 2);
					pop.setButton([param = utils.getParam(parametr.getPID())]()
					{
						param->setDefaultValue(param->getValue());
					}, 3);
					pop.setButton([param = utils.getParam(parametr.getPID())]()
					{
						param->switchLock();
					}, 4);
					pop.setButton([&u = utils, pID = parametr.getPID()]()
					{
						u.assignMIDILearn(pID);
					}, 5);
					pop.setButton([&u = utils, pID = parametr.getPID()]()
					{
						u.removeMIDILearn(pID);
					}, 6);
					pop.setButton([&u = utils, &paramtr = parametr]()
					{
						u.getEventSystem().notify(EvtType::EnterParameterValue, &paramtr);
					}, 7);

					pop.place(&parametr);
				}
			};
		}
	
	public:
		PopUpKnobs(Utils& u) :
			PopUp(u)
		{
			evts.push_back({ utils.getEventSystem(), makeNotify2(*this) });

			buttons.reserve(7);
			addButton("Randomize", "Randomize this parameter value.");
			addButton("Rand Relative", "Randomize this parameter value relative to its current value.");
			addButton("Load Default", "Resets this parameter value to its default value.");
			addButton("Save Default", "Saves this parameter value as its default one.");
			addButton("Lock / Unlock", "Parameter values are locked into place, even when changing presets.");
			addButton("MIDI Learn", "Click here to assign this parameter to a hardware control.");
			addButton("MIDI Unlearn", "Click here to remove this parameter from its hardware control(s).");
			addButton("Enter Value", "Click here to enter a parameter value with your keyboard");

			init();
		}
	};
}