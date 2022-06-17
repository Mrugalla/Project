#pragma once
#include "Comp.h"
#include "../arch/Interpolation.h"

namespace gui
{
	struct SplineEditor :
		public Comp
	{
		using Points = std::vector<PointF>;

		SplineEditor(Utils& u, const String& _tooltip) :
			Comp(u, _tooltip, CursorType::Interact),
			bounds(),
			pointsRel(),
			pointsAbs(),
			curve()
		{
			updateCurve();
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
				const auto pt = pointsAbs[i];
				g.drawRoundedRectangle
				(
					pt.x - thicc,
					pt.y - thicc,
					thicc2,
					thicc2,
					thicc,
					thicc
				);
			}
		}

		void resized() override
		{
			bounds = getLocalBounds().toFloat().reduced(utils.thicc * 2.f);
			updateCurve();
		}

		bool addPoint(const PointF& ptRel) noexcept
		{
			auto pt = limitRel(ptRel);

			if (existsRel(pt))
				return false;

			pointsRel.push_back(pt);
			pointsAbs.push_back(toAbs(ptRel));
			
			return true;
		}

		size_t numPoints() const noexcept { return pointsRel.size(); }

		void updateAbs()
		{
			pointsAbs.resize(numPoints());
			for (auto i = 0; i < numPoints(); ++i)
				pointsAbs[i] = toAbs(pointsRel[i]);
		}

		void updateCurve()
		{
			curve.clear();
			sort();
			updateAbs();

			if (!pointsRel.empty())
			{
				const auto func = interpolate::polynomial::getFunc(pointsAbs);
				const auto thicc = utils.thicc;
				
				{
					const auto x = limitAbsX(0.f);
					const auto y = limitAbsY(func(x));
					curve.startNewSubPath(x, y);
				}

				for (auto x = 1.f; x < bounds.getWidth(); x += thicc)
				{
					const auto y = func(x);
					curve.lineTo(limitAbsX(x), limitAbsY(y));
				}
			}
		}

	protected:
		BoundsF bounds;
		Points pointsRel, pointsAbs;
		Path curve;

		// CONTROL:

		void mouseDown(const Mouse& mouse) override
		{

		}
		void mouseDrag(const Mouse& mouse) override
		{

		}
		void mouseUp(const Mouse& mouse) override
		{
			if (mouse.mouseWasDraggedSinceMouseDown())
			{
				
			}
			else
			{
				if (mouse.mods.isLeftButtonDown())
				{
					addPoint(toRel(mouse.position));
				}
				else if(mouse.mods.isRightButtonDown())
				{
					const auto newSize = static_cast<int>(numPoints()) - 1;
					if(newSize != -1)
						pointsRel.resize(newSize);
				}
				updateCurve();
				repaint();
			}
		}

		// EVALUATION:

		bool existsAbs(const PointF& pt)
		{
			for (const auto& pAbs : pointsAbs)
				if (pt.x == pAbs.x)
					return true;
			return false;
		}

		bool existsRel(const PointF& pt)
		{
			for (const auto& pRel : pointsRel)
				if (pt.x == pRel.x)
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
			auto sortFunc = [](const PointF& pt0, const PointF& pt1)
			{
				return pt0.x < pt1.x;
			};

			std::sort(pointsRel.begin(), pointsRel.end(), sortFunc);
		}
	};
}