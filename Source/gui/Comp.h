#pragma once
#include "Utils.h"
#include "Layout.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{
	struct Comp :
		public Component
	{
		Comp(Utils&, const String& /*_tooltip*/ = "", CursorType = CursorType::Interact);

		Comp(Utils&, const String& /*_tooltip*/, Notify&&, CursorType = CursorType::Interact);

		const Utils& getUtils() const noexcept;
		Utils& getUtils() noexcept;

		const String* getTooltip() const noexcept;
		String* getTooltip() noexcept;

		void setTooltip(String&&);
	
		void setCursorType(CursorType);

		void updateCursor();

		const Layout& getLayout() const noexcept;
		
		void initLayout(const std::vector<int>& /*xL*/, const std::vector<int>& /*yL*/);
		
		void initLayout(const String& /*xL*/, const String& /*yL*/);

		void notify(EvtType, const void* = nullptr);

	protected:
		Utils& utils;
		Layout layout;
		std::vector<Evt> evts;
		String tooltip;
		CursorType cursorType;

		void paint(Graphics&) override;

		void mouseEnter(const Mouse&) override;

		void mouseUp(const Mouse&) override;

	private:
		Notify makeNotifyBasic(Comp*);

	};

	struct CompWidgetable :
		public Comp,
		public Timer
	{
		CompWidgetable(Utils&, String&& /*_tooltip*/, CursorType = CursorType::Interact);
		
		CompWidgetable(Utils&, String&& /*_tooltip*/,
			Notify&& = [](EvtType, const void*) {}, CursorType = CursorType::Interact);

		void defineBounds(const BoundsF&, const BoundsF&);

		void initWidget(float /*lengthInSecs*/, bool /*_widgetEnv*/ = false);

		void updateBounds();

		void timerCallback() override;
		
		BoundsF bounds0, bounds1;
		float widgetEnvelope;
	private:
		float widgetInc;
	};

	struct ScrollBar :
		public Comp
	{
		ScrollBar(Utils& u, Comp& _comp) :
			Comp(u, "Drag / Mousewheel to scroll."),
			comp(_comp),
			handleSize(1.f),
			handlePos(0.f)
		{
		}

		void resized() override
		{
			const auto numChildren = comp.getNumChildComponents();
			const auto h = static_cast<float>(getHeight());

			auto maxBottom = h;
			for (auto c = 0; c < numChildren; ++c)
			{
				const auto child = comp.getChildComponent(c);
				const auto btm = static_cast<float>(child->getBottom());

				if (maxBottom < btm)
					maxBottom = btm;
			}

			if (maxBottom == 0.f)
			{
				handleSize = 1.f;
				handlePos = 0.f;
				return repaint();
			}

			handleSize = std::max(.02f, std::min(h / maxBottom, 1.f));
		}

		bool isNeeded() const noexcept { return handleSize < 1.f; }

		float getHandlePos() const noexcept { return handlePos; }
	protected:
		Comp& comp;
		float handleSize, handlePos;

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc();
			const auto w = static_cast<float>(getWidth());
			const auto h = static_cast<float>(getHeight());

			const auto handleHeight = h * handleSize;
			const auto handleY = (h - handleHeight) * handlePos;

			const auto bounds = BoundsF(0.f, handleY, w, handleHeight).reduced(thicc);

			g.setColour(Colours::c(ColourID::Hover));
			if (isMouseOver())
				g.fillRoundedRectangle(bounds, thicc);
			if (isMouseButtonDown())
				g.fillRoundedRectangle(bounds, thicc);

			g.setColour(Colours::c(ColourID::Interact));
			g.drawRoundedRectangle(bounds, thicc, thicc);
		}

		void mouseEnter(const Mouse& mouse) override
		{
			Comp::mouseEnter(mouse);
			repaint();
		}
		void mouseDown(const Mouse& mouse) override
		{
			updateHandlePos(mouse.position.y);
		}
		void mouseDrag(const Mouse& mouse) override
		{
			updateHandlePos(mouse.position.y);
			repaint();
		}
		void mouseUp(const Mouse&) override
		{
			repaint();
		}
		void mouseExit(const Mouse&) override
		{
			repaint();
		}

		void updateHandlePos(float y)
		{
			const auto h = static_cast<float>(getHeight());
			handlePos = juce::jlimit(0.f, 1.f, y / h);

			getParentComponent()->resized();
		}
	};

	struct CompScrollable :
		public Comp
	{
		CompScrollable(Utils& u, Comp& _comp) :
			Comp(u, "", CursorType::Default),
			comp(_comp),
			scrollBar(u, comp)
		{
			layout.init({ 21, 1 }, { 1 });

			addAndMakeVisible(comp);
			addAndMakeVisible(scrollBar);
		}

	protected:
		Comp& comp;
		ScrollBar scrollBar;

		void resized() override
		{
			layout.resized();

			// might resize itself to large than getHeight()
			
			layout.place(comp, 0, 0, 1, 1, false);

			// registers height difference between comp and this
			layout.place(scrollBar, 1, 0, 1, 1, false);

			auto y = -scrollBar.getHandlePos() * static_cast<float>(comp.getHeight());
			comp.setBounds(
				0,
				static_cast<int>(y),
				getWidth(),
				comp.getHeight()
			);
		}

		void paint(Graphics&) override {}
	};
}