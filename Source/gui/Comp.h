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

	struct CompScrollable :
		public Comp
	{
		struct ScrollBar :
			public Comp
		{
			ScrollBar(Utils& u, CompScrollable& _scrollable) :
				Comp(u, "Drag / Mousewheel to scroll."),
				scrollable(_scrollable),
				handleSize(1.f)
			{
			}

			void resized() override
			{
				const auto numChildren = scrollable.getNumChildComponents();
				const auto h = static_cast<float>(getHeight());

				auto maxBottom = h;
				for (auto c = 0; c < numChildren; ++c)
				{
					const auto child = scrollable.getChildComponent(c);
					const auto btm = static_cast<float>(child->getBottom());

					if (maxBottom < btm)
						maxBottom = btm;
				}

				if (maxBottom == 0.f)
				{
					handleSize = 1.f;
					return repaint();
				}

				handleSize = std::max(.02f, std::min(h / maxBottom, 1.f));
			}

			bool isNeeded() const noexcept { return handleSize < 1.f; }
		protected:
			CompScrollable& scrollable;
			float handleSize;

			void paint(Graphics& g) override
			{
				const auto thicc = utils.thicc();
				const auto w = static_cast<float>(getWidth());
				const auto h = static_cast<float>(getHeight());

				const auto handleHeight = h * handleSize;
				const auto handleY = (h - handleHeight) * scrollable.yScrollOffset;

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
				scrollable.yScrollOffset = juce::jlimit(0.f, 1.f, y / h);

				getParentComponent()->resized();
			}
		};

		CompScrollable(Utils& u) :
			Comp(u, "", CursorType::Default),
			scrollBar(u, *this),
			xScrollOffset(0.f), yScrollOffset(0.f)
		{
			addAndMakeVisible(scrollBar);
		}

	protected:
		ScrollBar scrollBar;
		float xScrollOffset, yScrollOffset;
	};

	struct CompScreenshotable :
		public Comp
	{
		using PPFunc = std::function<void(Graphics&, Image&)>;

		CompScreenshotable(Utils& u) :
			Comp(u, "", CursorType::Default),
			screenshotImage(Image::RGB, 1, 1, false),
			onScreenshotFX()
		{
			setOpaque(true);
		}

		void resized() override
		{
			screenshotImage = screenshotImage.rescaled(
				getWidth(),
				getHeight(),
				Graphics::lowResamplingQuality
			);
			screenshotImage.clear(getLocalBounds(), Colours::c(ColourID::Bg));
		}

		void paint(Graphics& g) override
		{
			g.drawImageAt(screenshotImage, 0, 0, false);
		}
		
		void takeScreenshot()
		{
			screenshotImage = utils.pluginTop.createComponentSnapshot(
				getBounds(),
				true
			);
			Graphics g{ screenshotImage };
			for (auto& ossfx : onScreenshotFX)
				ossfx(g, screenshotImage);
		}

	protected:
		Image screenshotImage;
		std::vector<PPFunc> onScreenshotFX;
	};
}