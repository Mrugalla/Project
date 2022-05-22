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
			static constexpr float SensitiveDrag = .2f;
			static constexpr float WheelDefaultSpeed = 12.f;

			ScrollBar(Utils& u, CompScrollable& _scrollable) :
				Comp(u, "Drag / Mousewheel to scroll."),
				scrollable(_scrollable),
				dragXY(0.f)
			{
				setBufferedToImage(true);
			}

			bool needed() const noexcept
			{
				return scrollable.actualHeight > static_cast<float>(getHeight());
			}

		protected:
			CompScrollable& scrollable;
			float dragXY;

			void paint(Graphics& g) override
			{
				if (!needed())
					return;

				const auto w = static_cast<float>(scrollable.getWidth());
				const auto h = static_cast<float>(scrollable.getHeight());

				const auto thicc = utils.thicc;

				auto handleHeight = h / scrollable.actualHeight * h;
				if (handleHeight < thicc)
					handleHeight = thicc;

				const auto handleY = scrollable.yScrollOffset / scrollable.actualHeight * (h - handleHeight);

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
				if (!needed())
					return;

				hideCursor();
				const auto h = static_cast<float>(scrollable.getHeight());
				dragXY = mouse.position.y / utils.getDragSpeed() * h;
			}

			void mouseDrag(const Mouse& mouse) override
			{
				if (!needed())
					return;

				const auto h = static_cast<float>(scrollable.getHeight());
				const auto nDragXY = mouse.position.y / utils.getDragSpeed() * h;
				auto dragDif = nDragXY - dragXY;
				if (mouse.mods.isShiftDown())
					dragDif *= SensitiveDrag;
				updateHandlePos(scrollable.yScrollOffset + dragDif);
				dragXY = nDragXY;
			}

			void mouseUp(const Mouse& mouse) override
			{
				if (!needed())
					return;

				const auto h = static_cast<float>(scrollable.getHeight());
				
				if (mouse.mouseWasDraggedSinceMouseDown())
				{
					const auto nDragXY = mouse.position.y / utils.getDragSpeed() * h;
					const auto dragDif = nDragXY - dragXY;
					updateHandlePos(scrollable.yScrollOffset + dragDif);
					showCursor(*this);
				}
				else
				{
					const auto relPos = mouse.y / h;
					updateHandlePos(relPos * scrollable.actualHeight);
					const auto pos = mouse.position.toInt();
					showCursor(*this, &pos);
				}
			}

			void mouseExit(const Mouse&) override
			{
				repaint();
			}

			void mouseWheelMove(const Mouse& mouse, const juce::MouseWheelDetails& wheel) override
			{
				const auto reversed = wheel.isReversed ? -1.f : 1.f;
				const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
				auto dragY = 0.f;
				if (isTrackPad)
				{
					dragY = reversed * wheel.deltaY;
				}
				else
				{
					const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
					dragY = reversed * deltaYPos;
				}
				if (mouse.mods.isShiftDown())
					dragY *= SensitiveDrag;
				dragY *= utils.thicc * WheelDefaultSpeed;
				updateHandlePos(scrollable.yScrollOffset - dragY);
			}

			void updateHandlePos(float y)
			{
				const auto h = static_cast<float>(scrollable.getHeight());
				const auto maxHeight = std::max(h, scrollable.actualHeight - h);
				scrollable.yScrollOffset = juce::jlimit(0.f, maxHeight, y);
				getParentComponent()->resized();
				repaint();
			}
		};

		CompScrollable(Utils& u) :
			Comp(u, "", CursorType::Default),
			scrollBar(u, *this),
			xScrollOffset(0.f),
			yScrollOffset(0.f),
			actualHeight(1.f)
		{
			addAndMakeVisible(scrollBar);
		}

	protected:
		ScrollBar scrollBar;
		float xScrollOffset, yScrollOffset, actualHeight;
	};

	struct CompScreenshotable :
		public Comp
	{
		using PPFunc = std::function<void(Graphics&, Image&)>;

		CompScreenshotable(Utils& u) :
			Comp(u, "", CursorType::Default),
			screenshotImage(),
			onScreenshotFX()
		{
			setOpaque(true);
		}

		void resized() override
		{
			if (screenshotImage.isNull())
			{
				screenshotImage = Image(Image::RGB, getWidth(), getHeight(), false);
			}
			else
			{
				screenshotImage = screenshotImage.rescaled(
					getWidth(),
					getHeight(),
					Graphics::lowResamplingQuality
				);
			}
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