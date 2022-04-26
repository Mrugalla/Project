#pragma once
#include "Label.h"

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
			label.textCID = ColourID::Interact;
			addAndMakeVisible(label);
		}

		void enableLabel(std::vector<String>&& txt)
		{
			toggleTexts = txt;
			label.setText("");
			label.textCID = ColourID::Interact;
			addAndMakeVisible(label);
		}

		void enableParameterSwitch(PID pID)
		{
			stopTimer();

			onClick.push_back([param = utils.getParam(pID)]()
			{
				const auto ts = param->getValue() > .5f ? 0.f : 1.f;
				param->setValueWithGesture(ts);
			});
			
			onTimer.push_back([this, pID]()
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
			});

			startTimerHz(24);
		}

		void enableParameter(PID pID, int val)
		{
			stopTimer();

			onClick.push_back([param = utils.getParam(pID), v = static_cast<float>(val)]()
			{
				const auto pVal = std::rint(param->getValueDenorm());
				const auto ts = pVal == v ? 0.f : v;
				param->setValueWithGesture(param->range.convertTo0to1(ts));
			});

			onTimer.push_back([this, pID, val]()
			{
				const auto param = utils.getParam(pID);
				const auto pVal = std::rint(param->getValueDenorm());
				const auto nTs = static_cast<int>(pVal);
				if (toggleState != nTs)
				{
					toggleState = nTs;

					if (toggleTexts.size() > toggleState)
						label.setText(toggleTexts[toggleState]);
					repaintWithChildren(this);
				}
			});

			startTimerHz(24);
		}

		Button(Utils& _utils, String&& _tooltip) :
			Comp(_utils, _tooltip, makeNotify(this)),
			onClick(),
			onTimer(),
			onPaint(),
			blinkyBoy(),
			toggleState(-1),
			label(utils, ""),
			toggleTexts()
		{

		}

		Label& getLabel() noexcept { return label; }

		std::vector<OnClick> onClick, onTimer;
		std::vector<OnPaint> onPaint;
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
			for(auto& op: onPaint)
				op(g, *this);
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
			for(auto& oc: onClick)
				oc();
			notify(EvtType::ButtonClicked, this);
		}

		void timerCallback() override
		{
			for(auto& ot: onTimer)
				ot();
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

	inline void makeTextButton(Button& b, String&& txt, bool withToggle = false, int targetToggleState = 1)
	{
		b.enableLabel(std::move(txt));

		b.onPaint.push_back([withToggle, targetToggleState](Graphics& g, Button& button)
			{
				const auto& utils = button.getUtils();
				const auto& blinkyBoy = button.blinkyBoy;

				auto thicc = utils.thicc();
				const auto thiccHalf = thicc * .5f;
				const bool isOver = button.isMouseOver();
				const bool isDown = button.isMouseButtonDown();
				thicc *= (isOver ? 1.1f : 1.f);

				const auto area = button.getLocalBounds().toFloat().reduced(thiccHalf);

				const auto col = blinkyBoy.getInterpolated(Colours::c(ColourID::Bg), juce::Colours::white);

				g.setColour(col);
				g.fillRoundedRectangle(area, thicc);

				g.setColour(Colours::c(ColourID::Hover));
				if (withToggle && button.toggleState == targetToggleState)
					g.fillRoundedRectangle(area, thicc);

				g.drawRoundedRectangle(area, thicc, thicc);

				if (button.isMouseOver())
				{
					g.fillRoundedRectangle(area, thicc);
					if (isDown)
						g.fillRoundedRectangle(area, thicc);
				}
		});
	}

	enum class ButtonSymbol
	{
		Polarity,
		StereoConfig,
		UnityGain,
		Power,
		PatchMode,
		Settings,
		NumSymbols
	};

	inline void makeSymbolButton(Button& b, ButtonSymbol symbol, int targetToggleState = 1)
	{
		bool withToggle = true;
		if (symbol == ButtonSymbol::PatchMode)
			withToggle = false;
		else if (symbol == ButtonSymbol::StereoConfig)
		{
			withToggle = false;
			b.enableLabel({ "L/R", "M/S" });
		}
		
		b.onPaint.push_back([symbol, withToggle, targetToggleState](Graphics& g, Button& button)
		{
			const auto& utils = button.getUtils();
			const auto& blinkyBoy = button.blinkyBoy;

			auto thicc = utils.thicc();
			const bool isOver = button.isMouseOver();
			const bool isDown = button.isMouseButtonDown();
			thicc *= (isOver ? 1.1f : 1.f);

			auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
			auto col = blinkyBoy.getInterpolated(Colours::c(ColourID::Bg), juce::Colours::white);

			g.setColour(col);
			g.fillRoundedRectangle(bounds, thicc);

			g.setColour(Colours::c(ColourID::Hover));
			if (withToggle && button.toggleState == targetToggleState)
				g.fillRoundedRectangle(bounds, thicc);
			g.drawRoundedRectangle(bounds, thicc, thicc);

			if (button.isMouseOver())
			{
				g.fillRoundedRectangle(bounds, thicc);
				if (isDown)
					g.fillRoundedRectangle(bounds, thicc);
			}

			bool abortable = symbol == ButtonSymbol::Settings;
			if (abortable && button.toggleState == 1)
				col = Colours::c(ColourID::Abort);
			else
				col = Colours::c(ColourID::Interact);
			g.setColour(col);

			if (symbol == ButtonSymbol::Polarity)
			{
				const auto thicc3 = thicc * 3.f;

				bounds = maxQuadIn(bounds).reduced(thicc3);
				g.drawEllipse(bounds, thicc);

				const LineF line(bounds.getBottomLeft(), bounds.getTopRight());
				g.drawLine(line, thicc);
			}
			else if (symbol == ButtonSymbol::UnityGain)
			{
				const auto thicc3 = thicc * 3.f;

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
			else if (symbol == ButtonSymbol::Power)
			{
				const auto thicc3 = thicc * 3.f;

				bounds = bounds.reduced(thicc3);

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

				g.setColour(Colours::c(ColourID::Interact));
				g.drawLine(line, thicc);
			}
			else if (symbol == ButtonSymbol::PatchMode)
			{
				if (button.toggleState == 0)
				{
					const auto thicc3 = thicc * 3.f;
					bounds = maxQuadIn(bounds).reduced(thicc3);

					g.drawEllipse(bounds, thicc);
					const auto rad = bounds.getWidth() * .5f;
					PointF centre(
						bounds.getX() + rad,
						bounds.getY() + rad
					);
					const auto tick = LineF::fromStartAndAngle(centre, rad, PiQuart);
					g.drawLine(tick, thicc);
				}
				else
				{
					const auto thicc3 = thicc * 2.f;
					bounds = maxQuadIn(bounds).reduced(thicc3);

					const auto x0 = bounds.getX();
					const auto y0 = bounds.getY() + bounds.getHeight() * .5f;
					const auto x1 = x0 + bounds.getWidth() * .2f;
					const auto y1 = y0;
					g.drawLine(x0, y0, x1, y1, thicc);
					const auto x2 = x0 + bounds.getWidth() * .3f;
					const auto yA = bounds.getY() + bounds.getHeight() * .2f;
					const auto yB = bounds.getBottom() - bounds.getHeight() * .2f;
					g.drawLine(x1, y1, x2, yA, thicc);
					g.drawLine(x1, y1, x2, yB, thicc);
					const auto x3 = x0 + bounds.getWidth() * .7f;
					g.drawLine(x2, yA, x3, yA, thicc);
					g.drawLine(x2, yB, x3, yB, thicc);
					const auto x4 = x0 + bounds.getWidth() * .8f;
					const auto y4 = y0;
					g.drawLine(x3, yA, x4, y4, thicc);
					g.drawLine(x3, yB, x4, y4, thicc);
					const auto x5 = bounds.getRight();
					g.drawLine(x4, y4, x5, y4, thicc);
				}
			}
			else if (symbol == ButtonSymbol::Settings)
			{
				if (button.toggleState == 1)
				{
					const auto thicc3 = thicc * 3.f;
					bounds = maxQuadIn(bounds).reduced(thicc3);

					g.setFont(getFontNEL());
					g.drawFittedText("X", bounds.toNearestInt(), Just::centred, 1, 0.f);
				}
				else
				{
					const auto thicc3 = thicc * 4.f;
					bounds = maxQuadIn(bounds).reduced(thicc3);

					const auto x = bounds.getX();
					const auto y = bounds.getY();
					const auto w = bounds.getWidth();
					const auto h = bounds.getHeight();
					const auto btm = y + h;
					const auto rght = x + w;

					juce::PathStrokeType stroke(
						thicc,
						juce::PathStrokeType::JointStyle::curved,
						juce::PathStrokeType::EndCapStyle::rounded
					);

					const auto tickWidth = .2f;
					const auto rad = w * tickWidth;
					const auto angle0 = 0.f - PiQuart;
					const auto angle1 = PiHalf + PiQuart;
					
					{
						const auto centreX = x;
						const auto centreY = btm;

						Path path;
						path.addCentredArc(
							centreX, centreY,
							rad, rad,
							0.f, angle0, angle1,
							true
						);

						g.strokePath(path, stroke);
					}

					{
						const auto centreX = rght;
						const auto centreY = y;
						Path path;
						path.addCentredArc(
							centreX, centreY,
							rad, rad,
							Pi, angle0, angle1,
							true
						);

						g.strokePath(path, stroke);
					}

					{
						const auto padding = rad;

						const auto x0 = x + padding;
						const auto y0 = btm - padding;
						const auto x1 = rght - padding;
						const auto y1 = y + padding;

						g.drawLine(x0, y0, x1, y1, thicc);
					}
				}
			}
		});
	}

	inline void makeParameterSwitchButton(Button& b, PID pID, String&& txt)
	{
		makeTextButton(b, std::move(txt), true);
		b.enableParameterSwitch(pID);
	}

	inline void makeParameterSwitchButton(Button& b, PID pID, ButtonSymbol symbol)
	{
		makeSymbolButton(b, symbol);
		b.enableParameterSwitch(pID);
	}

	template<size_t NumButtons>
	inline void makeParameterButtonsGroup(std::array<Button, NumButtons>& btns, PID pID, const char* txt, bool onlyText)
	{
		for (auto i = 0; i < NumButtons; ++i)
		{
			auto& btn = btns[i];

			const auto ts = i + 1;

			makeTextButton(btn, String::charToString(txt[i]), true, onlyText, ts);
			btn.enableParameter(pID, ts);
		}
			
	}

	inline void makeButtonsGroup(std::vector<std::unique_ptr<Button>>& btns, int defaultToggleStateIndex = 0)
	{
		for (auto& btn : btns)
			btn->toggleState = 0;
		btns[defaultToggleStateIndex]->toggleState = 1;

		for (auto i = 0; i < btns.size(); ++i)
		{
			auto& btn = *btns[i];

			btn.onClick.push_back([&buttons = btns, i]()
			{
				for (auto& btn : buttons)
				{
					btn->toggleState = 0;
					repaintWithChildren(btn.get());
				}
					
				buttons[i]->toggleState = 1;
			});
		}
	}

	inline void makeURLButton(Button& b, String&& urlPath)
	{
		const juce::URL url(urlPath);

		b.onClick.push_back([url]()
		{
			url.launchInDefaultBrowser();
		});
	}
}

/*

toggleState == 1
	has glow

*/