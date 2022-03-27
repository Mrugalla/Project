#pragma once
#include "Comp.h"

namespace gui
{
	struct BlinkyBoy :
		public Timer
	{
		BlinkyBoy() :
			comp(nullptr),
			env(0.f),
			inv(0.f)
		{}
		void init(Comp* _comp, float timeInSecs) noexcept
		{
			const auto fps = 30.f;
			comp = _comp;
			env = 1.f;
			inv = 1.f / (timeInSecs * fps);
			startTimerHz(static_cast<int>(fps));
		}
		Colour getInterpolated(Colour c0, Colour c1) const noexcept
		{
			const auto e = env * env;
			return c0.interpolatedWith(c1, e < 0.f ? 0.f : e);
		}
	protected:
		Comp* comp;
		float env, inv;

		void timerCallback() override
		{
			env -= inv;
			if (env < 0.f)
				stopTimer();
			comp->repaint();
		}
	};

	struct Button :
		public Comp,
		public Timer
	{
		using OnClick = std::function<void()>;
		using OnPaint = std::function<void(Graphics&, Button&)>;

		void enableLabel(String&& txt)
		{
			label.setText(txt);
			label.bgCID = ColourID::Transp;
			label.outlineCID = ColourID::Transp;
			label.textCID = ColourID::Interact;
			addAndMakeVisible(label);
		}

		void enableLabel(std::vector<String>&& txt)
		{
			toggleTexts = txt;
			label.setText("");
			label.bgCID = ColourID::Transp;
			label.outlineCID = ColourID::Transp;
			label.textCID = ColourID::Interact;
			addAndMakeVisible(label);
		}

		void enableParameterSwitch(PID pID)
		{
			stopTimer();

			onClick = [param = utils.getParam(pID)]()
			{
				const auto ts = param->getValue() > .5f ? 0 : 1;
				param->setValueWithGesture(static_cast<float>(ts));
			};
			
			utils.getParam(pID);

			onTimer = [this, pID]()
			{
				const auto param = utils.getParam(pID);
				const auto nTs = param->getValue() > .5f ? 1 : 0;
				if (toggleState != nTs)
				{
					toggleState = nTs;

					if(toggleTexts.size() > toggleState)
						label.setText(toggleTexts[toggleState]);
					repaintWithChildren(this);
				}
			};

			startTimerHz(24);
		}

		Button(Utils& _utils, String&& _tooltip) :
			Comp(_utils, _tooltip, makeNotify(this)),
			onClick([]() {}),
			onTimer([]() {}),
			onPaint([](Graphics&, Button&) {}),
			blinkyBoy(),
			toggleState(-1),
			label(utils, ""),
			toggleTexts()
		{

		}

		Label& getLabel() noexcept { return label; }

		OnClick onClick, onTimer;
		OnPaint onPaint;
		BlinkyBoy blinkyBoy;
		int toggleState;
	protected:
		Label label;
		std::vector<String> toggleTexts;

		void resized() override
		{
			if (label.isVisible())
			{
				const auto thicc = utils.thicc();
				const auto thicc4 = thicc * 2.f;
				const auto bounds = getLocalBounds().toFloat().reduced(thicc4);

				label.setBounds(bounds.toNearestInt());
			}
		}

		void paint(Graphics& g) override
		{
			onPaint(g, *this);
		}

		void mouseEnter(const Mouse& mouse) override
		{
			Comp::mouseEnter(mouse);
			repaint();
		}
		void mouseExit(const Mouse&) override
		{
			repaint();
		}
		void mouseUp(const Mouse& mouse) override
		{
			if (mouse.mouseWasDraggedSinceMouseDown())
				return;
			blinkyBoy.init(this, .25f);
			onClick();
			notify(EvtType::ButtonClicked, this);
		}

		void timerCallback() override
		{
			onTimer();
		}

	private:
		Notify makeNotify(Button* button)
		{
			return [button](const EvtType, const void*)
			{
				
			};
		}
	};

	inline Button::OnPaint buttonOnPaintDefault()
	{
		return [](Graphics& g, Button& button)
		{
			auto col = button.blinkyBoy.getInterpolated(juce::Colours::darkgrey, juce::Colours::white);
			g.fillAll(col);
		};
	}

	inline void makeTextButton(Button& b, String&& txt, bool withToggle = false, bool onlyText = false)
	{
		b.enableLabel(std::move(txt));

		if (onlyText)
			b.onPaint = [withToggle](Graphics& g, Button& button)
			{
				const auto& utils = button.getUtils();
				const auto& blinkyBoy = button.blinkyBoy;

				auto thicc = utils.thicc();
				const bool isOver = button.isMouseOver();
				const bool isDown = button.isMouseButtonDown();
				thicc *= (isOver ? 1.1f : 1.f);

				auto area = button.getLocalBounds().toFloat().reduced(thicc);
				const auto col = blinkyBoy.getInterpolated(Colours::c(ColourID::Transp), juce::Colours::white);

				g.setColour(col);
				g.fillRoundedRectangle(area, thicc);

				if (withToggle && button.toggleState == 1)
				{
					g.setColour(Colours::c(ColourID::Hover));
					g.fillRoundedRectangle(area, thicc);
				}

				if (button.isMouseOver())
				{
					g.setColour(Colours::c(ColourID::Hover));
					g.fillRoundedRectangle(area, thicc);
					if (isDown)
						g.fillRoundedRectangle(area, thicc);
				}
			};
		else
			b.onPaint = [withToggle](Graphics& g, Button& button)
			{
				const auto& utils = button.getUtils();
				const auto& blinkyBoy = button.blinkyBoy;

				auto thicc = utils.thicc();
				const bool isOver = button.isMouseOver();
				const bool isDown = button.isMouseButtonDown();
				thicc *= (isOver ? 1.1f : 1.f);

				auto area = button.getLocalBounds().toFloat().reduced(thicc);

				const auto col = blinkyBoy.getInterpolated(Colours::c(ColourID::Bg), juce::Colours::white);

				g.setColour(col);
				g.fillRoundedRectangle(area, thicc);

				if (withToggle && button.toggleState == 1)
				{
					g.setColour(Colours::c(ColourID::Hover));
					g.fillRoundedRectangle(area, thicc);
				}

				if (button.isMouseOver())
				{
					g.setColour(Colours::c(ColourID::Hover));
					g.fillRoundedRectangle(area, thicc);
					if (isDown)
						g.fillRoundedRectangle(area, thicc);
				}
				g.setColour(Colours::c(ColourID::Interact));
				g.drawRoundedRectangle(area, thicc, thicc);
			};
	}

	enum class ButtonSymbol
	{
		Polarity,
		StereoConfig,
		UnityGain,
		Bypass,
		NumSymbols
	};

	inline void makeSymbolButton(Button& b, ButtonSymbol symbol, bool withToggle = false)
	{
		if (symbol == ButtonSymbol::StereoConfig)
		{
			withToggle = false;
			b.enableLabel({ "L/R", "M/S" });
		}
		
		b.onPaint = [symbol, withToggle](Graphics& g, Button& button)
		{
			const auto& utils = button.getUtils();
			const auto& blinkyBoy = button.blinkyBoy;

			auto thicc = utils.thicc();
			const bool isOver = button.isMouseOver();
			const bool isDown = button.isMouseButtonDown();
			thicc *= (isOver ? 1.1f : 1.f);

			auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
			const auto col = blinkyBoy.getInterpolated(Colours::c(ColourID::Bg), juce::Colours::white);

			g.setColour(col);
			g.fillRoundedRectangle(bounds, thicc);

			if (withToggle && button.toggleState == 1)
			{
				g.setColour(Colours::c(ColourID::Hover));
				g.fillRoundedRectangle(bounds, thicc);
			}

			if (button.isMouseOver())
			{
				g.setColour(Colours::c(ColourID::Hover));
				g.fillRoundedRectangle(bounds, thicc);
				if (isDown)
					g.fillRoundedRectangle(bounds, thicc);
			}
			g.setColour(Colours::c(ColourID::Interact));
			g.drawRoundedRectangle(bounds, thicc, thicc);

			if (symbol == ButtonSymbol::Polarity)
			{
				const auto thicc3 = thicc * 2.f;

				bounds = maxQuadIn(bounds).reduced(thicc3);
				g.drawEllipse(bounds, thicc);

				const LineF line(bounds.getBottomLeft(), bounds.getTopRight());
				g.drawLine(line, thicc);
			}
			else if (symbol == ButtonSymbol::UnityGain)
			{
				const auto thicc3 = thicc * 2.f;

				bounds = bounds.reduced(thicc3);

				const auto x0 = bounds.getX();
				const auto y0 = bounds.getY();
				
				const auto w = bounds.getWidth() * .666f;
				const auto h = bounds.getHeight() * .666f;

				const auto x1 = x0 + w * .5f;
				const auto y1 = y0 + h * .5f;

				g.drawEllipse({ x0, y0, w, h }, thicc);
				g.drawEllipse({ x1, y1, w, h }, thicc);
			}
			else if (symbol == ButtonSymbol::Bypass)
			{
				const auto thicc3 = thicc * 2.f;

				//const auto y0 = bounds.getY();

				bounds = bounds.reduced(thicc3);

				//g.drawEllipse(bounds, thicc);
				
				const auto x = bounds.getX();
				const auto y = bounds.getY();
				const auto rad = bounds.getWidth() * .5f;

				const PointF centre(
					x + rad,
					y + rad
				);

				const auto pi = 3.14159265359f;

				const auto fromRads = pi * .2f;
				const auto toRads = 2.f * pi - fromRads;

				Path path;
				path.addCentredArc(
					centre.x,
					centre.y,
					rad,
					rad,
					0.f,
					fromRads,
					toRads,
					true
				);

				g.strokePath(path, juce::PathStrokeType(thicc));

				const LineF line(centre, centre.withY(y));

				//g.setColour(Colours::c(ColourID::Bg));
				//g.drawLine(line.withShortenedEnd(-thicc), thicc3);
				g.setColour(Colours::c(ColourID::Interact));
				g.drawLine(line, thicc);
			}
		};
	}

	inline void makeParameterSwitchButton(Button& b, PID pID, String&& txt)
	{
		makeTextButton(b, std::move(txt), true);
		b.enableParameterSwitch(pID);
	}

	inline void makeParameterSwitchButton(Button& b, PID pID, ButtonSymbol symbol)
	{
		makeSymbolButton(b, symbol, true);
		b.enableParameterSwitch(pID);
	}
}

/*

toggleState == 1
	has glow

*/