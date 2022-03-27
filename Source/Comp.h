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
			notify(utils.getEventSystem()),
			tooltip(_tooltip),
			notifyBasic(utils.getEventSystem(), makeNotifyBasic(this)),
			cursorType(_cursorType)
		{
			setMouseCursor(makeCursor(cursorType));
		}
		Comp(Utils& _utils, const String& _tooltip, Notify&& _notify, CursorType _cursorType = CursorType::Interact) :
			utils(_utils),
			layout(*this),
			notify(utils.getEventSystem(), _notify),
			tooltip(_tooltip),
			notifyBasic(utils.getEventSystem(), makeNotifyBasic(this)),
			cursorType(_cursorType)
		{
			setMouseCursor(makeCursor(cursorType));
		}

		const Utils& getUtils() const noexcept { return utils; }
		Utils& getUtils() noexcept { return utils; }

		const String* getTooltip() const noexcept { return &tooltip; }
		String* getTooltip() noexcept { return &tooltip; }
	
		void setCursorType(CursorType ct)
		{
			if (cursorType != ct)
			{
				cursorType = ct;
				setMouseCursor(makeCursor(cursorType));
			}
		}

	protected:
		Utils& utils;
		Layout layout;
		const Evt notify;
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

	private:
		const Evt notifyBasic;

		Notify makeNotifyBasic(Comp* c)
		{
			return [c](const EvtType type, const void*)
			{
				if (type == EvtType::ColourShemeChanged)
				{
					// c->update mouse cursor
					c->repaint();
				}
			};
		}
	};

	struct CompWidgetable :
		public Comp
	{
		CompWidgetable(Utils& u) :
			Comp(u)
		{} // to do
	};
}