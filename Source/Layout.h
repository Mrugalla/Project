#pragma once
#include <array>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{
	using Component = juce::Component;
	using Path = juce::Path;
	using Point = juce::Point<int>;
	using PointF = juce::Point<float>;
	using Bounds = juce::Rectangle<int>;
	using BoundsF = juce::Rectangle<float>;
	using Line = juce::Line<int>;
	using LineF = juce::Line<float>;
	using Affine = juce::AffineTransform;
	using Just = juce::Justification;

	inline BoundsF maxQuadIn(const BoundsF& b) noexcept
	{
		const auto minDimen = std::min(b.getWidth(), b.getHeight());
		const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
		const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
		return { x, y, minDimen, minDimen };
	}

	inline void repaintWithChildren(Component* comp)
	{
		comp->repaint();
		for (auto c = 0; c < comp->getNumChildComponents(); ++c)
			repaintWithChildren(comp->getChildComponent(c));
	}

	inline std::unique_ptr<juce::XmlElement> loadXML(const char* data, const int sizeInBytes)
	{
		return juce::XmlDocument::parse(String(data, sizeInBytes));
	}

	class Layout
	{
		using Bounds = juce::Rectangle<float>;
		using BoundsInt = juce::Rectangle<int>;
	public:
		Layout(const Component& _comp) :
			comp(_comp),
			rXRaw(),
			rYRaw(),
			rX(),
			rY()
		{
		}
		void init(const std::vector<int>& xDist, const std::vector<int>& yDist)
		{
			const auto numCols = xDist.size();
			const auto numRows = yDist.size();
			rXRaw.reserve(numCols + 2);
			rYRaw.reserve(numRows + 2);
			
			rXRaw.emplace_back(0.f);
			rYRaw.emplace_back(0.f);

			{ // normalize input values Cols
				auto sum = 0.f;
				for (auto i = 0; i < numCols; ++i)
				{
					rXRaw.emplace_back(static_cast<float>(xDist[i]));
					sum += static_cast<float>(xDist[i]);
				}
				const auto g = 1.f / sum;
				for (auto& i : rXRaw)
					i *= g;
			}
			{ // normalize input values Rows
				auto sum = 0.f;
				for (auto i = 0; i < numRows; ++i)
				{
					rYRaw.emplace_back(static_cast<float>(yDist[i]));
					sum += static_cast<float>(yDist[i]);
				}
				const auto g = 1.f / sum;
				for (auto& i : rYRaw)
					i *= g;
			}

			rXRaw.emplace_back(0.f);
			rYRaw.emplace_back(0.f);

			for (auto x = 1; x < rXRaw.size(); ++x)
				rXRaw[x] += rXRaw[x - 1];
			for (auto y = 1; y < rYRaw.size(); ++y)
				rYRaw[y] += rYRaw[y - 1];

			rX = rXRaw;
			rY = rYRaw;
		}
		void fromStrings(const String& xStr, const String& yStr)
		{
			std::vector<int> xDist, yDist;

			int sIdx = 0;
			for (auto i = 0; i < xStr.length(); ++i)
			{
				if (xStr[i] == ';')
				{
					xDist.push_back(xStr.substring(sIdx, i).getIntValue());
					++i;
					sIdx = i;
				}
			}
			xDist.push_back(xStr.substring(sIdx).getIntValue());
			sIdx = 0;
			for (auto i = 0; i < yStr.length(); ++i)
			{
				if (yStr[i] == ';')
				{
					yDist.push_back(yStr.substring(sIdx, i).getIntValue());
					++i;
					sIdx = i;
				}
			}
			yDist.push_back(yStr.substring(sIdx).getIntValue());

			init(xDist, yDist);
		}

		void resized() noexcept
		{
			const auto bounds = comp.getLocalBounds().toFloat();
			for (auto x = 0; x < rX.size(); ++x)
				rX[x] = rXRaw[x] * bounds.getWidth();
			for (auto y = 0; y < rY.size(); ++y)
				rY[y] = rYRaw[y] * bounds.getHeight();
			for (auto& x : rX)
				x += bounds.getX();
			for (auto& y : rY)
				y += bounds.getY();
		}

		template<typename X, typename Y>
		PointF operator()(X x, Y y) const noexcept
		{
			return { getX(x), getY(y) };
		}
		template<typename PointType>
		PointF operator()(PointType pt) const noexcept
		{
			return { getX(pt.x), getY(pt.y) };
		}
		template<typename X, typename Y>
		Bounds operator()(X x, Y y, X width, Y height, bool isQuad = false) const noexcept
		{
			const auto x0 = getX(x);
			const auto y0 = getY(y);

			Bounds nBounds(x0, y0, getX(x + width) - x0, getY(y + height) - y0);
			return isQuad ? maxQuadIn(nBounds) : nBounds;
		}

		template<typename PointType0, typename PointType1>
		LineF getLine(PointType0 p0, PointType1 p1) const noexcept
		{
			return { getX(p0.x), getY(p0.y), getX(p1.x), getY(p1.y) };
		}
		template<typename X0, typename Y0, typename X1, typename Y1>
		LineF getLine(X0 x0, Y0 y0, X1 x1, Y1 y1) const noexcept
		{
			return { getX(x0), getY(y0), getX(x1), getY(y1) };
		}

		Bounds bottom(bool isQuad = false) const noexcept
		{
			const auto w = rX.back();
			const auto y = getY(static_cast<int>(rY.size() - 3));
			const auto h = getY(static_cast<int>(rY.size() - 2)) - y;
			Bounds nBounds(0.f, y, w, h);
			return isQuad ? maxQuadIn(nBounds) : nBounds;
		}
		Bounds top(bool isQuad = false) const noexcept
		{
			const auto w = rX.back();
			const auto h = getY(1);
			Bounds nBounds(0.f, 0.f, w, h);
			return isQuad ? maxQuadIn(nBounds) : nBounds;
		}
		Bounds right(bool isQuad = false) const noexcept
		{
			const auto y = 0.f;
			const auto x = getX(static_cast<int>(rX.size() - 3));
			const auto w = getX(static_cast<int>(rX.size() - 2)) - x;
			const auto h = rY.back();

			Bounds nBounds(x, y, w, h);
			return isQuad ? maxQuadIn(nBounds) : nBounds;
		}

		float getX(int i) const noexcept { return rX[i]; }
		float getY(int i) const noexcept { return rY[i]; }
		float getX(float i) const noexcept
		{
			const auto f = std::floor(i);
			const auto iF = static_cast<int>(f);
			const auto iC = iF + 1;
			const auto x = i - f;

			const auto xF = getX(iF);
			const auto xC = getX(iC);

			return xF + x * (xC - xF);
		}
		float getY(float i) const noexcept
		{
			const auto f = std::floor(i);
			const auto iF = static_cast<int>(f);
			const auto iC = iF + 1;
			const auto y = i - f;

			const auto yF = getY(iF);
			const auto yC = getY(iC);

			return yF + y * (yC - yF);
		}

		template<typename X, typename Y>
		void place(Component& childComp, X x, Y y, X width = static_cast<X>(1), Y height = static_cast<Y>(1), bool isQuad = false) const noexcept
		{
			const auto cBounds = this->operator()(x, y, width, height);
			if (!isQuad)
				childComp.setBounds(cBounds.toNearestInt());
			else
				childComp.setBounds(maxQuadIn(cBounds).toNearestInt());
		}
		template<typename X, typename Y>
		void place(Component* childComp, X x, Y y, X width = static_cast<X>(1), Y height = static_cast<Y>(1), bool isQuad = false) const noexcept
		{
			if (childComp == nullptr) return;
			place(*childComp, x, y, width, height, isQuad);
		}
		
		void paint(Graphics& g)
		{
			for (auto x = 0; x < rX.size(); ++x)
				g.drawVerticalLine(static_cast<int>(rX[x]), rY[0], static_cast<float>(comp.getBottom()));
			for (auto y = 0; y < rY.size(); ++y)
				g.drawHorizontalLine(static_cast<int>(rY[y]), rX[0], static_cast<float>(comp.getRight()));
		}
		template<typename X, typename Y>
		void label(Graphics& g, String&& txt, X x, Y y, X width = static_cast<X>(1), Y height = static_cast<Y>(1), bool isQuad = false)
		{
			const auto bounds = operator()(x, y, width, height, isQuad);
			g.drawRect(bounds);
			g.drawFittedText(txt, bounds.toNearestInt(), Just::centred, 1);
		}

	protected:
		const Component& comp;
		std::vector<float> rXRaw, rX;
		std::vector<float> rYRaw, rY;
	};

	inline void make(Path& path, const Layout& layout, std::vector<Point>&& points)
	{
		path.startNewSubPath(layout(points[0]));
		for (auto i = 1; i < points.size(); ++i)
			path.lineTo(layout(points[i]));
	}

	template<class CompType>
	inline void distributeVertically(const Component& parent, std::vector<std::unique_ptr<CompType>>& compArray)
	{
		const auto w = static_cast<float>(parent.getWidth());
		const auto h = static_cast<float>(parent.getHeight());
		const auto x = 0.f;

		auto y = 0.f;

		const auto cH = h / static_cast<float>(compArray.size());

		for (auto& cmp: compArray)
		{
			cmp->setBounds(BoundsF(x, y, w, cH).toNearestInt());
			y += cH;
		}
	}
	
	template<class CompType>
	inline void distributeVertically(const Component& parent, CompType* compArray, int size)
	{
		const auto w = static_cast<float>(parent.getWidth());
		const auto h = static_cast<float>(parent.getHeight());
		const auto x = 0.f;

		auto y = 0.f;

		const auto cH = h / static_cast<float>(size);

		for (auto i = 0; i < size; ++i)
		{
			compArray[i].setBounds(BoundsF(x, y, w, cH).toNearestInt());
			y += cH;
		}
	}
}

/*

layout reduced by text width and/or height
	for example for tooltips

*/