#pragma once
#include "Param.h"
#include "Label.h"
#include "GUIParams.h"

namespace gui
{
    struct Dial :
        public Parametr,
        public Timer
    {
        Dial(Utils& u, String&& _name, PID _pID, bool _modulatable = true) :
            Parametr(u, _pID, _modulatable),
            Timer(),
            label(u, std::move(_name)),
            dragY(0.f),
            dialBounds(0.f, 0.f, 0.f, 0.f)
        {
            layout.init(
                { 1 },
                { 50, 40 }
            );

            label.textCID = ColourID::Interact;
            label.just = Just::centredTop;

            setName(label.getName());
            addAndMakeVisible(label);
            startTimerHz(60);
        }

    protected:
        Label label;
        float dragY;
        BoundsF dialBounds;

        void timerCallback() override
        {
            const auto vn = param.getValue();
            if (valNorm != vn)
            {
                valNorm = vn;
                repaint();
            }
        }

        void paint(juce::Graphics& g) override
        {
            const auto thicc = utils.thicc();
            const auto thicc2 = thicc * 2.f;
            const auto thicc3 = thicc * 3.f;
            const auto bounds = dialBounds;
            juce::PathStrokeType strokeType(
                thicc,
                juce::PathStrokeType::JointStyle::curved,
                juce::PathStrokeType::EndCapStyle::rounded
            );

            //draw outline
            {
                const auto centreY = bounds.getY() + bounds.getHeight() * .5f;
                const auto left = bounds.getX();
                const auto right = bounds.getRight();

                const auto y0 = centreY - thicc;
                const auto y1 = centreY + thicc;

                Path path;
                
                path.startNewSubPath({ left, y0 });
                path.lineTo({ right, y0 });
                path.startNewSubPath({ left, y1 });
                path.lineTo({ right, y1 });
                
                g.setColour(Colours::c(ColourID::Interact));
                g.strokePath(path, strokeType);
            }

            // draw tick
            {
                const auto tickX = bounds.getX() + valNorm * bounds.getWidth();
                const auto top = bounds.getY();
                const auto bottom = bounds.getBottom();

                {
                    g.setColour(Colours::c(ColourID::Bg));
                    const auto x = static_cast<int>(tickX);
                    
                    g.drawVerticalLine(x, top, bottom);
                    for (auto i = 1; i < static_cast<int>(thicc3); ++i)
                    {
                        g.drawVerticalLine(x + i, top, bottom);
                        g.drawVerticalLine(x - i, top, bottom);
                    }
                }

                Path path;
                
                path.startNewSubPath({ tickX, top });
                path.lineTo({ tickX, bottom });

                g.setColour(Colours::c(ColourID::Interact));

                strokeType.setStrokeThickness(thicc2);

                strokeType.setEndStyle(juce::PathStrokeType::EndCapStyle::rounded);

                g.strokePath(path, strokeType);
            }
        }

        void resized() override
        {
            layout.resized();

            const auto thicc = utils.thicc();

            if (label.empty())
            {
                dialBounds = layout(0, 0, 1, 2, false).reduced(thicc);
            }
            else
            {
                dialBounds = layout(0, 0, 1, 1, false).reduced(thicc * 2.f);
                layout.place(label, 0, 1, 1, 1, false);
            }
            
        }

        void mouseEnter(const Mouse& mouse) override
        {
            Comp::mouseEnter(mouse);
        }
        void mouseDown(const Mouse& mouse) override
        {
            if (mouse.mods.isLeftButtonDown())
            {
                const auto width = static_cast<float>(getWidth());
                PointF nPt(0.f, getHeight() * .5f);
                {
                    auto& desktop = juce::Desktop::getInstance();
                    auto mouseSrc = desktop.getMainMouseSource();
                    nPt.x = width * valNorm;
                    mouseSrc.setScreenPosition(getScreenPosition().toFloat() + nPt);
                }

                param.beginGesture();
                dragY = nPt.x / width;
            }
        }
        void mouseDrag(const Mouse& mouse) override
        {
            if (mouse.mods.isLeftButtonDown())
            {
                const auto width = static_cast<float>(getWidth());
                const auto dragYNew = mouse.position.x / width;
                auto dragOffset = dragYNew - dragY;
                if (mouse.mods.isShiftDown())
                    dragOffset *= SensitiveDrag;
                const auto newValue = juce::jlimit(0.f, 1.f, param.getValue() + dragOffset);
                param.setValueNotifyingHost(newValue);
                dragY = dragYNew;
            }
        }
        void mouseUp(const Mouse& mouse) override
        {
            if (mouse.mods.isLeftButtonDown())
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                {
                    if (mouse.mods.isCtrlDown())
                        param.setValueNotifyingHost(param.getDefaultValue());
                    
                }
                param.endGesture();
            }
            else if (mouse.mods.isRightButtonDown())
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isCtrlDown())
                        param.setValueWithGesture(param.getDefaultValue());
        }
        void mouseWheelMove(const Mouse& mouse, const juce::MouseWheelDetails& wheel) override
        {
            const bool reversed = wheel.isReversed ? -1.f : 1.f;
            const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
            if (isTrackPad)
                dragY = reversed * wheel.deltaY;
            else
            {
                const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
                dragY = reversed * WheelDefaultSpeed * deltaYPos;
            }
            if (mouse.mods.isShiftDown())
                dragY *= SensitiveDrag;
            const auto newValue = juce::jlimit(0.f, 1.f, param.getValue() + dragY);
            param.setValueWithGesture(newValue);
        }

    };
}