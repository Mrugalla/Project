#pragma once

namespace gui
{
    struct Shader :
        juce::ImageEffectFilter
    {
        Shader(const bool& _bypassed) :
            bypassed(_bypassed)
        {}

        void applyEffect(Image& img, Graphics& g, float, float)
        {
            g.drawImageAt(img, 0, 0, false);

            if (bypassed)
                paintBypassed(img, g);
        }

    protected:
        const bool& bypassed;

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
    };

}