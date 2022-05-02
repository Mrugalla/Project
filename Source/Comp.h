#pragma once
#include "Utils.h"
#include "Events.h"
#include "Layout.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{
	struct Comp :
		Component
	{
		Comp(Utils& _utils, const String& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
			utils(_utils),
			layout(*this),
			evts(),
			tooltip(_tooltip),
			cursorType(_cursorType)
		{
			evts.reserve(1);
			evts.emplace_back(utils.getEventSystem(), makeNotifyBasic(this));

			setMouseCursor(makeCursor(cursorType));
		}
		Comp(Utils& _utils, const String& _tooltip, Notify&& _notify, CursorType _cursorType = CursorType::Interact) :
			utils(_utils),
			layout(*this),
			evts(),
			tooltip(_tooltip),
			cursorType(_cursorType)
		{
			evts.reserve(2);
			evts.emplace_back(utils.getEventSystem(), makeNotifyBasic(this));
			evts.emplace_back(utils.getEventSystem(), _notify);

			setMouseCursor(makeCursor(cursorType));
		}

		const Utils& getUtils() const noexcept { return utils; }
		Utils& getUtils() noexcept { return utils; }

		const String* getTooltip() const noexcept { return &tooltip; }
		String* getTooltip() noexcept { return &tooltip; }

		void setTooltip(String&& t)
		{
			tooltip = t;
			setInterceptsMouseClicks(true, true);
		}
	
		void setCursorType(CursorType ct)
		{
			if (cursorType != ct)
			{
				cursorType = ct;
				updateCursor();
			}
		}
		void updateCursor()
		{
			setMouseCursor(makeCursor(cursorType));
		}

		const Layout& getLayout() const noexcept { return layout; }
		void initLayout(const std::vector<int>& xL, const std::vector<int>& yL)
		{
			layout.init(xL, yL);
		}
		void initLayout(const String& xL, const String& yL)
		{
			layout.fromStrings(xL, yL);
		}

		void notify(EvtType type, const void* stuff = nullptr)
		{
			evts[0](type, stuff);
		}

	protected:
		Utils& utils;
		Layout layout;
		std::vector<Evt> evts;
		String tooltip;
		CursorType cursorType;

		void paint(Graphics& g) override
		{
			g.setColour(Colour(0xffff0000));
			g.drawRect(getLocalBounds().toFloat(), 1.f);
		}

		void mouseEnter(const Mouse&) override
		{
			notify(EvtType::TooltipUpdated, &tooltip);
		}

		void mouseUp(const Mouse&) override
		{
			notify(EvtType::ClickedEmpty, this);
		}

	private:
		Notify makeNotifyBasic(Comp* c)
		{
			return [c](const EvtType type, const void*)
			{
				if (type == EvtType::ColourSchemeChanged)
				{
					c->updateCursor();
					c->repaint();
				}
			};
		}
	};

	struct CompWidgetable :
		public Comp,
		public Timer
	{
		CompWidgetable(Utils& u, String&& _tooltip, CursorType _cursorType = CursorType::Interact) :
			Comp(u, std::move(_tooltip), _cursorType),
			bounds0(),
			bounds1(),
			widgetEnvelope(0.f),
			widgetInc(1.f)
		{
		}
		
		CompWidgetable(Utils& u, String&& _tooltip, Notify&& _notify = [](EvtType,const void*){}, CursorType _cursorType = CursorType::Interact) :
			Comp(u, std::move(_tooltip), std::move(_notify), _cursorType),
			bounds0(),
			bounds1(),
			widgetEnvelope(0.f),
			widgetInc(1.f)
		{
		}

		void defineBounds(const BoundsF& b0, const BoundsF& b1)
		{
			bounds0 = b0;
			bounds1 = b1;
		}

		void initWidget(float lengthInSecs, bool _widgetEnv = false)
		{
			widgetEnvelope = _widgetEnv ? 1.f : 0.f;
			widgetInc = 1.f / (30.f * lengthInSecs) * (_widgetEnv ? -1.f : 1.f);
			startTimerHz(30);
		}

		void updateBounds()
		{
			const auto x = static_cast<int>(bounds0.getX() + widgetEnvelope * (bounds1.getX() - bounds0.getX()));
			const auto y = static_cast<int>(bounds0.getY() + widgetEnvelope * (bounds1.getY() - bounds0.getY()));
			const auto w = static_cast<int>(bounds0.getWidth() + widgetEnvelope * (bounds1.getWidth() - bounds0.getWidth()));
			const auto h = static_cast<int>(bounds0.getHeight() + widgetEnvelope * (bounds1.getHeight() - bounds0.getHeight()));

			setBounds(x, y, w, h);
		}

		void timerCallback() override
		{
			widgetEnvelope += widgetInc;
			if(widgetEnvelope < 0.f || widgetEnvelope > 1.f)
			{
				stopTimer();
				widgetEnvelope = std::rint(widgetEnvelope);
			}
			
			updateBounds();
		}
		
		BoundsF bounds0, bounds1;
		float widgetEnvelope;
	private:
		float widgetInc;
	};
}