#pragma once
#include "Comp.h"
#include "../arch/Interpolation.h"
#include "../arch/Range.h"

namespace gui
{
	struct SplineEditor :
		public Comp,
		public Timer
	{
		static constexpr float MinDraggerWidth = .01f;
		static constexpr float DraggerWidthStep = .01f;

		enum XY{ X, Y };

		struct SplinePoint
		{
			SplinePoint(const PointF& _rel) :
				rel(_rel),
				selected(false)
			{}

			PointF rel;
			bool selected;
		};

		using Points = std::vector<SplinePoint>;

		struct DraggerFall
		{
			DraggerFall(SplineEditor& e) :
				editor(e),
				posX(-1.f),
				width(MinDraggerWidth)
			{
			}

			void setPosition(float p)
			{
				posX = p;
			}

			void addWidth(float w)
			{
				width = juce::jlimit(MinDraggerWidth, 1.f, width + w);
			}

			void paint(Graphics& g)
			{
				if (posX == -1.f)
					return;

				const auto absWidth = editor.toAbsX(width);
				const auto editorRight = editor.bounds.getRight();

				auto x = editor.toAbsX(posX) - absWidth;
				auto right = x + absWidth * 2.f;
				if (x < editor.bounds.getX())
					x = editor.bounds.getX();
				if (right >= editorRight)
					right = editorRight;
				auto w = right - x;

				const auto y = editor.bounds.getY();
				const auto h = editor.bounds.getHeight();

				g.fillRoundedRectangle(x, y, w, h, editor.utils.thicc);
			}

			bool isSelectable(float xRel) const noexcept
			{
				return xRel >= posX - width && xRel <= posX + width;
			}

		protected:
			SplineEditor& editor;
			float posX, width;
		};

		struct Grid
		{
			Grid(SplineEditor& e, bool _snap) :
				editor(e),
				range
				{
					makeRange::stepped(0.f, 1.f, 1.f / 16.f),
					makeRange::stepped(0.f, 1.f, 1.f / 16.f)
				},
				snap(_snap)
			{}

			void paint(Graphics& g)
			{
				const auto gridSteps = 16.f;

				const auto left = editor.bounds.getX();
				const auto right = editor.bounds.getRight();
				const auto top = editor.bounds.getY();
				const auto bottom = editor.bounds.getBottom();

				const auto inc = 1.f / gridSteps;
				for (auto r = 0.f; r < 1.f; r += inc)
				{
					const auto yAbs = static_cast<int>(editor.toAbsY(r));
					g.drawHorizontalLine(yAbs, left, right);

					const auto xAbs = static_cast<int>(editor.toAbsX(r));
					g.drawVerticalLine(xAbs, top, bottom);
				}
			}

			PointF applySnap(const PointF& pRel) const noexcept
			{
				if (snap)
					return	{ range[X].snapToLegalValue(pRel.x), range[X].snapToLegalValue(pRel.y) };
				return { juce::jlimit(0.f, 1.f, pRel.x), juce::jlimit(0.f, 1.f, pRel.y) };
			}
		protected:
			SplineEditor& editor;
			std::array<RangeF, 2> range;
			bool snap;
		};

		SplineEditor(Utils& u, const String& _tooltip) :
			Comp(u, _tooltip, CursorType::Interact),
			bounds(),
			points(),
			curve(),
			drag(*this),
			grid(*this, false),
			wannaUpdate(false),
			dragXY()
		{
			updateCurve();

			startTimerHz(24);
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc;
			const auto thicc2 = thicc * 2.f;
			const Stroke stroke(thicc, Stroke::JointStyle::beveled, Stroke::EndCapStyle::butt);

			auto col = Colours::c(ColourID::Hover);
			g.setColour(col);
			g.fillRoundedRectangle(bounds, thicc);
			
			col = Colours::c(ColourID::Interact);
			g.setColour(col);
			g.strokePath(curve, stroke);

			for (auto i = 0; i < numPoints(); ++i)
			{
				const auto pt = grid.applySnap(points[i].rel);

				const auto ptRelX = pt.x;

				const auto isSelectable = drag.isSelectable(ptRelX);
				const auto cornerSize = isSelectable ? thicc2 * 1.5f : thicc2;

				const auto ptAbs = toAbs(pt);
				
				g.drawRoundedRectangle
				(
					ptAbs.x - thicc,
					ptAbs.y - thicc,
					cornerSize,
					cornerSize,
					thicc,
					thicc
				);
			}

			g.setColour(Colours::c(ColourID::Hover));
			drag.paint(g);

			grid.paint(g);
		}

		void resized() override
		{
			bounds = getLocalBounds().toFloat().reduced(utils.thicc * 2.f);
			updateCurve();
		}

		bool addPoint(const PointF& ptRel) noexcept
		{
			const auto pt = limitRel(ptRel);

			if (exists(pt))
				return false;

			points.push_back({ pt });
			
			return true;
		}

		size_t numPoints() const noexcept
		{
			return points.size();
		}

		void updateCurve()
		{
			curve.clear();
			sort();

			if (points.empty())
				return;

			std::vector<PointF> ptAbs;
			ptAbs.reserve(numPoints());
			for (const auto& pt : points)
				ptAbs.emplace_back(toAbs(grid.applySnap(pt.rel)));

			const auto func = interpolate::polynomial::getFunc(ptAbs);
			const auto thicc2 = utils.thicc * 2.f;


			auto x = limitAbsX(bounds.getX());
			auto y = limitAbsY(func(x));
			curve.startNewSubPath(x, y);

			while (x < bounds.getRight())
			{
				y = func(x);
				curve.lineTo(limitAbsX(x), limitAbsY(y));

				x += thicc2;
			}
			
			x = bounds.getRight();
			y = func(x);
			curve.lineTo(limitAbsX(x), limitAbsY(y));
		}

		void timerCallback() override
		{
			if (!wannaUpdate.load())
				return;

			updateCurve();
			wannaUpdate.store(false);
			repaint();
		}

		void deselect() noexcept
		{
			for (auto i = 0; i < numPoints(); ++i)
				points[i].selected = false;
		}

	protected:
		BoundsF bounds;
		Points points;
		Path curve;
		DraggerFall drag;
		Grid grid;
		std::atomic<bool> wannaUpdate;
		PointF dragXY;

		// CONTROL:

		void mouseExit(const Mouse&) override
		{
			drag.setPosition(-1.f);
			repaint();
		}
		void mouseMove(const Mouse& mouse) override
		{
			drag.setPosition(toRelX(mouse.position.x));
			repaint();
		}
		void mouseDown(const Mouse& mouse) override
		{
			dragXY = mouse.position;
			
			bool needsRepaint = false;

			for (auto i = 0; i < numPoints(); ++i)
			{
				if (isSelectableRel(points[i].rel.x))
				{
					points[i].selected = true;
					needsRepaint = true;
				}
			}

			if (needsRepaint)
				repaint();
		}

		void mouseDrag(const Mouse& mouse) override
		{
			drag.setPosition(toRelX(mouse.position.x));
			if (selectionExists())
			{
				const auto nDragXY = mouse.position;
				const auto distXY = nDragXY - dragXY;

				for(auto& pt: points)
					if (pt.selected)
					{
						const auto x = pt.rel.x;
						const auto y = pt.rel.y;

						pt.rel.x = toRelX(toAbsX(x) + distXY.x);
						pt.rel.y = toRelY(toAbsY(y) + distXY.y);
					}

				dragXY = nDragXY;
				wannaUpdate.store(true);
			}
			else
				repaint();
		}
		
		void mouseUp(const Mouse& mouse) override
		{
			bool needsUpdate = false;

			if (mouse.mouseWasDraggedSinceMouseDown())
			{
				
			}
			else
			{
				if (mouse.mods.isLeftButtonDown())
				{
					if (addPoint(toRel(mouse.position)))
						needsUpdate = true;
				}
				else if(mouse.mods.isRightButtonDown())
				{
					for (auto i = 0; i < numPoints(); ++i)
					{
						const auto sel = points[i].selected;
						if (sel)
						{
							points.erase(points.begin() + i);
							--i;
							needsUpdate = true;
						}
					}
				}
			}

			deselect();
			repaint();
			wannaUpdate.store(needsUpdate);
		}
		void mouseWheelMove(const Mouse&, const MouseWheel& wheel) override
		{
			const auto rev = wheel.isReversed ? -1.f : 1.f;
			const auto moveY = wheel.deltaY * rev;
			drag.addWidth(moveY > 0.f ? DraggerWidthStep : -DraggerWidthStep);
			repaint();
		}

		// EVALUATION:

		bool isSelectableRel(float xRel) const noexcept
		{
			return drag.isSelectable(xRel);
		}

		bool selectionExists() const noexcept
		{
			for (auto& pt : points)
				if (pt.selected)
					return true;
			return false;
		}

		bool exists(const PointF& pt) noexcept
		{
			for (const auto& pRel : points)
				if (pt.x == pRel.rel.x)
					return true;
			return false;
		}

		// CONVERSION:

		float limitAbsX(float x) const noexcept
		{
			return juce::jlimit(bounds.getX(), bounds.getRight(), x);
		}

		float limitAbsY(float y) const noexcept
		{
			return juce::jlimit(bounds.getY(), bounds.getBottom(), y);
		}

		PointF limitAbs(const PointF& pt) const noexcept
		{
			return{ limitAbsX(pt.x),limitAbsY(pt.y) };
		}

		float limitRel(float xy) const noexcept
		{
			return juce::jlimit(0.f, 1.f, xy);
		}

		PointF limitRel(const PointF& pt) const noexcept
		{
			return{ limitRel(pt.x),limitRel(pt.y) };
		}

		float toAbsX(float x) const noexcept
		{
			return bounds.getX() + x * bounds.getWidth();
		}

		float toAbsY(float y) const noexcept
		{
			return bounds.getY() + y * bounds.getHeight();
		}

		PointF toAbs(const PointF& pt) const noexcept
		{
			return { toAbsX(pt.x), toAbsY(pt.y) };
		}

		float toRelX(float x) const noexcept
		{
			return (x - bounds.getX()) / bounds.getWidth();
		}

		float toRelY(float y) const noexcept
		{
			return (y - bounds.getY()) / bounds.getHeight();
		}

		PointF toRel(const PointF& pt) const noexcept
		{
			return { toRelX(pt.x), toRelY(pt.y) };
		}

		// POST PROCESSING

		void sort()
		{
			const auto sortFunc = [](const SplinePoint& pt0, const SplinePoint& pt1)
			{
				return pt0.rel.x < pt1.rel.x;
			};

			std::sort(points.begin(), points.end(), sortFunc);
		}
	};

	struct SplineEditorPanel :
		public Comp
	{
		SplineEditorPanel(Utils& u, String&& _tooltip) :
			Comp(u, "", CursorType::Default),
			editor(u, _tooltip)
		{
			layout.init(
				{ 1 },
				{ 5, 1 }
			);

			addAndMakeVisible(editor);
		}

		void resized() override
		{
			layout.resized();

			layout.place(editor, 0, 0, 1, 1, false);
		}

		SplineEditor editor;
	};
}