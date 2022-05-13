#include "Layout.h"

gui::BoundsF gui::maxQuadIn(const BoundsF& b) noexcept
{
	const auto minDimen = std::min(b.getWidth(), b.getHeight());
	const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
	const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
	return { x, y, minDimen, minDimen };
}

void gui::repaintWithChildren(Component* comp)
{
	if (comp == nullptr)
		return;
	comp->repaint();
	for (auto c = 0; c < comp->getNumChildComponents(); ++c)
		repaintWithChildren(comp->getChildComponent(c));
}

std::unique_ptr<juce::XmlElement> gui::loadXML(const char* data, const int sizeInBytes)
{
	return juce::XmlDocument::parse(String(data, sizeInBytes));
}

gui::Layout::Layout(const Component& _comp) :
	comp(_comp),
	rXRaw(),
	rYRaw(),
	rX(),
	rY()
{
}

void gui::Layout::init(const std::vector<int>& xDist, const std::vector<int>& yDist)
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

void gui::Layout::fromStrings(const String& xStr, const String& yStr)
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

void gui::Layout::resized() noexcept
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
gui::PointF gui::Layout::operator()(X x, Y y) const noexcept
{
	return { getX(x), getY(y) };
}

template<typename PointType>
gui::PointF gui::Layout::operator()(PointType pt) const noexcept
{
	return { getX(pt.x), getY(pt.y) };
}

template<typename X, typename Y>
gui::BoundsF gui::Layout::operator()(X x, Y y, X width, Y height, bool isQuad) const noexcept
{
	const auto x0 = getX(x);
	const auto y0 = getY(y);

	Bounds nBounds(x0, y0, getX(x + width) - x0, getY(y + height) - y0);
	return isQuad ? maxQuadIn(nBounds) : nBounds;
}

template<typename PointType0, typename PointType1>
gui::LineF gui::Layout::getLine(PointType0 p0, PointType1 p1) const noexcept
{
	return { getX(p0.x), getY(p0.y), getX(p1.x), getY(p1.y) };
}

template<typename X0, typename Y0, typename X1, typename Y1>
gui::LineF gui::Layout::getLine(X0 x0, Y0 y0, X1 x1, Y1 y1) const noexcept
{
	return { getX(x0), getY(y0), getX(x1), getY(y1) };
}

gui::BoundsF gui::Layout::bottom(bool isQuad) const noexcept
{
	const auto w = rX.back();
	const auto y = getY(static_cast<int>(rY.size() - 3));
	const auto h = getY(static_cast<int>(rY.size() - 2)) - y;
	BoundsF nBounds(0.f, y, w, h);
	return isQuad ? maxQuadIn(nBounds) : nBounds;
}

gui::BoundsF gui::Layout::top(bool isQuad) const noexcept
{
	const auto w = rX.back();
	const auto h = getY(1);
	BoundsF nBounds(0.f, 0.f, w, h);
	return isQuad ? maxQuadIn(nBounds) : nBounds;
}

gui::BoundsF gui::Layout::right(bool isQuad) const noexcept
{
	const auto y = 0.f;
	const auto x = getX(static_cast<int>(rX.size() - 3));
	const auto w = getX(static_cast<int>(rX.size() - 2)) - x;
	const auto h = rY.back();

	BoundsF nBounds(x, y, w, h);
	return isQuad ? maxQuadIn(nBounds) : nBounds;
}

float gui::Layout::getX(int i) const noexcept { return rX[i]; }

float gui::Layout::getY(int i) const noexcept { return rY[i]; }

float gui::Layout::getX(float i) const noexcept
{
	const auto f = std::floor(i);
	const auto iF = static_cast<int>(f);
	const auto iC = iF + 1;
	const auto x = i - f;

	const auto xF = getX(iF);
	const auto xC = getX(iC);

	return xF + x * (xC - xF);
}

float gui::Layout::getY(float i) const noexcept
{
	const auto f = std::floor(i);
	const auto iF = static_cast<int>(f);
	const auto iC = iF + 1;
	const auto y = i - f;

	const auto yF = getY(iF);
	const auto yC = getY(iC);

	return yF + y * (yC - yF);
}

template<typename X>
float gui::Layout::getW(X i) const noexcept
{
	return getX(i + static_cast<X>(1)) - getX(i);
}

template<typename Y>
float gui::Layout::getH(Y i) const noexcept
{
	return getY(i + static_cast<Y>(1)) - getY(i);
}

template<typename X, typename Y>
void gui::Layout::place(Component& childComp, X x, Y y, X width, Y height, bool isQuad) const noexcept
{
	const auto cBounds = this->operator()(x, y, width, height);
	if (!isQuad)
		childComp.setBounds(cBounds.toNearestInt());
	else
		childComp.setBounds(maxQuadIn(cBounds).toNearestInt());
}

template<typename X, typename Y>
void gui::Layout::place(Component* childComp, X x, Y y, X width, Y height, bool isQuad) const noexcept
{
	if (childComp == nullptr) return;
	place(*childComp, x, y, width, height, isQuad);
}

void gui::Layout::paint(Graphics& g)
{
	for (auto x = 0; x < rX.size(); ++x)
		g.drawVerticalLine(static_cast<int>(rX[x]), rY[0], static_cast<float>(comp.getBottom()));
	for (auto y = 0; y < rY.size(); ++y)
		g.drawHorizontalLine(static_cast<int>(rY[y]), rX[0], static_cast<float>(comp.getRight()));
}

template<typename X, typename Y>
void gui::Layout::label(Graphics& g, String&& txt, X x, Y y, X width, Y height, bool isQuad)
{
	const auto bounds = operator()(x, y, width, height, isQuad);
	g.drawRect(bounds);
	g.drawFittedText(txt, bounds.toNearestInt(), Just::centred, 1);
}

void gui::make(Path& path, const Layout& layout, std::vector<Point>&& points)
{
	path.startNewSubPath(layout(points[0]));
	for (auto i = 1; i < points.size(); ++i)
		path.lineTo(layout(points[i]));
}

void gui::drawHorizontalLine(Graphics& g, int y, float left, float right, int thicc)
{
	g.drawHorizontalLine(y, left, right);
	for (auto t = 1; t < thicc; ++t)
	{
		g.drawHorizontalLine(y + t, left, right);
		g.drawHorizontalLine(y - t, left, right);
	}
}

void gui::drawVerticalLine(Graphics& g, int x, float top, float bottom, int thicc)
{
	g.drawVerticalLine(x, top, bottom);
	for (auto t = 1; t < thicc; ++t)
	{
		g.drawVerticalLine(x + t, top, bottom);
		g.drawVerticalLine(x - t, top, bottom);
	}
}

void gui::drawRectEdges(Graphics& g, const BoundsF& bounds,
	float edgeWidth, float edgeHeight, Stroke st)
{
	const auto x = bounds.getX();
	const auto y = bounds.getY();
	const auto right = bounds.getRight();
	const auto bottom = bounds.getBottom();

	const auto xPlusEdge = x + edgeWidth;
	const auto yPlusEdge = y + edgeHeight;
	const auto rightMinusEdge = right - edgeWidth;
	const auto bottomMinusEdge = bottom - edgeHeight;

	Path path;
	path.startNewSubPath(x, yPlusEdge);
	path.lineTo(x, y);
	path.lineTo(xPlusEdge, y);

	path.startNewSubPath(x, bottomMinusEdge);
	path.lineTo(x, bottom);
	path.lineTo(xPlusEdge, bottom);

	path.startNewSubPath(rightMinusEdge, bottom);
	path.lineTo(right, bottom);
	path.lineTo(right, bottomMinusEdge);

	path.startNewSubPath(right, yPlusEdge);
	path.lineTo(right, y);
	path.lineTo(rightMinusEdge, y);

	g.strokePath(path, st);
}

void gui::drawRectEdges(Graphics& g, const BoundsF& bounds,
	float edgeWidth, Stroke st)
{
	drawRectEdges(g, bounds, edgeWidth, edgeWidth, st);
}

void gui::drawHeadLine(Graphics& g, const BoundsF& bounds, const String& txt)
{
	g.drawFittedText(txt, bounds.toNearestInt(), Just::centredTop, 1);
}

template<class ArrayCompPtr>
void gui::distributeVertically(const Component& parent, ArrayCompPtr& compArray)
{
	const auto w = static_cast<float>(parent.getWidth());
	const auto h = static_cast<float>(parent.getHeight());
	const auto x = 0.f;

	auto y = 0.f;

	const auto cH = h / static_cast<float>(compArray.size());

	for (auto& cmp : compArray)
	{
		cmp->setBounds(BoundsF(x, y, w, cH).toNearestInt());
		y += cH;
	}
}

template<class SizeType>
void gui::distributeVertically(const Component& parent, Component* compArray, SizeType size)
{
	const auto w = static_cast<float>(parent.getWidth());
	const auto h = static_cast<float>(parent.getHeight());
	const auto x = 0.f;

	auto y = 0.f;

	const auto cH = h / static_cast<float>(size);

	for (auto i = 0; i < size; ++i)
	{
		auto& cmp = compArray[i];
		cmp.setBounds(BoundsF(x, y, w, cH).toNearestInt());
		y += cH;
	}
}

void gui::imgPP::blur(Image& img, Graphics& g, int its) noexcept
{
	const auto w = img.getWidth();
	const auto h = img.getHeight();

	std::array<Point, 4> pt(
		{
			{-1, -1}, {-1, -0},
			{-0, -1}, {-0, -0}
		});

	std::array<float, 3> rgb;
	enum { R, G, B };

	juce::Image::BitmapData bitmap(img, Image::BitmapData::ReadWriteMode::readWrite);

	for (auto j = 0; j < its; ++j)
		for (auto y = 1; y < h; ++y)
			for (auto x = 1; x < w; ++x)
			{
				rgb[R] = rgb[G] = rgb[B] = 0.f;
				for (auto i = 0; i < pt.size(); ++i)
				{
					const auto pxl = bitmap.getPixelColour(x + pt[i].x, y + pt[i].y);
					rgb[R] += pxl.getFloatRed();
					rgb[G] += pxl.getFloatGreen();
					rgb[B] += pxl.getFloatBlue();
				}
				for (auto i = 0; i < rgb.size(); ++i)
					rgb[i] *= .25f;

				const auto nPxl = Colour::fromFloatRGBA(rgb[R], rgb[G], rgb[B], 1.f);

				bitmap.setPixelColour(x, y, nPxl);
			}

	g.drawImageAt(img, 0, 0, false);
}