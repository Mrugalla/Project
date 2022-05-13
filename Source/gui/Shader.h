#pragma once
#include "Utils.h"

namespace gui
{
    struct Shader :
        juce::ImageEffectFilter,
        Timer
    {
        Shader(Utils& u, Component& _comp) :
            Timer(),
            comp(_comp),
            utils(u),
            notify(u.getEventSystem()),
            bypassed(false)
        {
            startTimerHz(12);
        }

        void applyEffect(Image& img, Graphics& g, float, float) override
        {
            g.drawImageAt(img, 0, 0, false);

            if (bypassed)
                paintBypassed(img, g);
        }

        Component& comp;
        Utils& utils;
        Evt notify;
        bool bypassed;

        void paintBypassed(Image& img, Graphics& g)
        {
            const auto h = static_cast<float>(img.getHeight()) * .5f;
            const auto r = static_cast<float>(img.getWidth());

            PointF left(0.f, h);
            PointF right(r, h);

            juce::ColourGradient grad(
                Colour(0x00000000),
                left,
                Colour(0xff000000),
                right,
                false
            );
            g.setGradientFill(grad);
            g.fillAll();
            g.setColour(Colours::c(ColourID::Abort));
            g.drawFittedText("bypassed", img.getBounds(), Just::centredRight, 1);
        }

        void timerCallback() override
        {
            bool shallRepaint = false;

            auto b = utils.getParam(PID::Power)->getValue() < .5f;
            if (bypassed != b)
            {
                bypassed = b;
                shallRepaint = true;
            }

            if (shallRepaint)
            {
                repaintWithChildren(&comp);
            }
        }
    };

    struct ShaderLowLevel :
        juce::ImageEffectFilter
    {
        Notify makeNotify(ShaderLowLevel& s)
        {
            return [&shader = s](EvtType type, const void*)
            {
                if (type == EvtType::BrowserOpened)
                {
                    shader.blur = true;
                    repaintWithChildren(&shader.comp);
                }
                else if (type == EvtType::BrowserClosed)
                {
                    shader.blur = false;
                    repaintWithChildren(&shader.comp);
                }
            };
        }

        ShaderLowLevel(Utils& u, Component& _comp) :
            comp(_comp),
            utils(u),
            notify(u.getEventSystem(), makeNotify(*this)),
            blur(false)
        {
        }

        void applyEffect(Image& img, Graphics& g, float, float) override
        {
            g.drawImageAt(img, 0, 0, false);

            if (blur)
                paintBlurred(img, g);
        }

        Component& comp;
        Utils& utils;
        Evt notify;
        bool blur;

        void paintBlurred(Image& img, Graphics& g)
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

            auto pxlData = img.getPixelData();
            
            juce::Image::BitmapData bitmap(img, Image::BitmapData::ReadWriteMode::readWrite);

            for(auto j = 0; j < 3; ++j)
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
                        for(auto i = 0; i < rgb.size(); ++i)
                            rgb[i] *= .25f;

                        const auto nPxl = Colour::fromFloatRGBA(rgb[R], rgb[G], rgb[B], 1.f);
                        
                        bitmap.setPixelColour(x, y, nPxl);
                    }

            g.drawImageAt(img, 0, 0, false);
        }
    };
}