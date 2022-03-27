#pragma once
#include "Layout.h"
#include "Param.h"
#include "Comp.h"
#include "Label.h"
#include "Shared.h"
#include "GUIParams.h"

#include "config.h"

namespace gui
{
    struct Knob :
        public Parametr,
        public Timer
    {
        static constexpr float AngleWidth = PiQuart * 3.f;
        static constexpr float AngleRange = AngleWidth * 2.f;

        Knob(Utils& u, String&& _name, PID _pID) :
            Parametr(u, _pID),
            Timer(),
            label(u, std::move(_name)),
            dragY(0.f),
            valMeter(0.f),
            knobBounds(0.f, 0.f, 0.f, 0.f)
        {
            layout.init(
                { 1 },
                { 80, 20 }
            );

            label.bgCID = ColourID::Transp;
            label.outlineCID = ColourID::Transp;
            label.textCID = ColourID::Interact;
            label.just = Just::centredTop;

            setName(std::move(_name));
            addAndMakeVisible(label);
            startTimerHz(PPDFPSKnobs);
        }

        void updateMeter(float v)
        {
            valMeter = v;
            repaint();
        }

    protected:
        Label label;
        float dragY, valMeter;
        BoundsF knobBounds;

        void timerCallback() override
        {
            const auto vn = param.getValue();
            const auto lckd = param.isLocked();

            bool needsRepaint = false;

            if (locked != lckd)
            {
                locked = lckd;
                label.textCID = lckd ? ColourID::Inactive : ColourID::Interact;
                setCursorType(lckd ? CursorType::Inactive : CursorType::Interact);
                label.repaint();
                needsRepaint = true;
            }
            
            if (valNorm != vn)
            {
                valNorm = vn;
                needsRepaint = true;
            }

            if (needsRepaint)
                repaint();
        }
        
        void paint(juce::Graphics& g) override
        {
            const auto thicc = utils.thicc();
            const auto thicc2 = thicc * 2.f;
            const auto thicc3 = thicc * 3.f;
            const auto bounds = knobBounds;
            juce::PathStrokeType strokeType(thicc, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
            const auto radius = bounds.getWidth() * .5f;
            const auto radiusBetween = radius - thicc;
            const auto radiusInner = radius - thicc2;
            PointF centre(
                radius + bounds.getX(),
                radius + bounds.getY()
            );

            auto col = locked ? Colours::c(ColourID::Inactive) : Colours::c(ColourID::Interact);

            if (valMeter != 0.f)
            {
                g.setColour(Colours::c(ColourID::Txt));
                Path meterArc;

                const auto meterAngle = AngleRange * valMeter - AngleWidth;

                meterArc.addCentredArc(
                    centre.x, centre.y,
                    radiusBetween, radiusBetween,
                    0.f,
                    -AngleWidth, meterAngle,
                    true
                );

                strokeType.setStrokeThickness(thicc2);
                g.strokePath(meterArc, strokeType);
                strokeType.setStrokeThickness(thicc);
            }

            //draw outlines
            {
                g.setColour(col);
                Path outtaArc;

                outtaArc.addCentredArc(
                    centre.x, centre.y,
                    radius, radius,
                    0.f,
                    -AngleWidth, AngleWidth,
                    true
                );
                outtaArc.addCentredArc(
                    centre.x, centre.y,
                    radiusInner, radiusInner,
                    0.f,
                    -AngleWidth, AngleWidth,
                    true
                );

                g.strokePath(outtaArc, strokeType);
            }

            const auto valNormAngle = valNorm * AngleRange;
            const auto valAngle = -AngleWidth + valNormAngle;
            const auto radiusExt = radius + thicc;

            // draw tick
            {
                const auto tickLine = juce::Line<float>::fromStartAndAngle(centre, radiusExt, valAngle);
                g.setColour(Colours::c(ColourID::Bg));
                g.drawLine(tickLine, thicc3);
                g.setColour(col);
                g.drawLine(tickLine.withShortenedStart(radiusInner), thicc2);
            }
        }
        
        void resized() override
        {
            const auto thicc = utils.thicc();
            
            layout.resized();

            knobBounds = layout(0, 0, 1, 1, true).reduced(thicc);
            layout.place(label, 0, 1, 1, 1, false);
        }

        void mouseEnter(const Mouse& mouse) override
        {
            Comp::mouseEnter(mouse);
            label.setText(param.getCurrentValueAsText());
            label.repaint();
        }
        void mouseExit(const Mouse&) override
        {
            label.setText(getName());
            label.repaint();
        }
        void mouseDown(const Mouse& mouse) override
        {
            if (mouse.mods.isLeftButtonDown())
            {
                param.beginGesture();
                dragY = mouse.position.y / utils.getDragSpeed();
                {
                    label.setText(param.getCurrentValueAsText());
                    label.repaint();
                }
            }
        }
        void mouseDrag(const Mouse& mouse) override
        {
            if (mouse.mods.isLeftButtonDown())
            {
                const auto dragYNew = mouse.position.y / utils.getDragSpeed();
                auto dragOffset = dragYNew - dragY;
                if (mouse.mods.isShiftDown())
                    dragOffset *= SensitiveDrag;
                const auto newValue = juce::jlimit(0.f, 1.f, param.getValue() - dragOffset);
                param.setValueNotifyingHost(newValue);
                dragY = dragYNew;
                {
                    label.setText(param.getCurrentValueAsText());
                    label.repaint();
                }
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
                    else
                    {
                        juce::Point<float> centre(
                            static_cast<float>(getWidth()) * .5f,
                            static_cast<float>(getHeight()) * .5f
                        );
                        const juce::Line<float> fromCentre(centre, mouse.position);
                        const auto angle = fromCentre.getAngle();

                        const auto newValue = juce::jlimit(0.f, 1.f, (angle + AngleWidth) / AngleRange);
                        param.setValue(newValue);
                    }
                }
                param.endGesture();
            }
            else if (mouse.mods.isRightButtonDown())
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isCtrlDown())
                        param.setValueWithGesture(param.getDefaultValue());
            {
                label.setText(param.getCurrentValueAsText());
                label.repaint();
            }
        }
        void mouseWheelMove(const Mouse& mouse, const juce::MouseWheelDetails& wheel) override
        {
            if (mouse.mods.isAnyMouseButtonDown())
                return;
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
            {
                label.setText(param.getCurrentValueAsText());
                label.repaint();
            }
        }

    };

    class KnobMeter :
        public Timer
    {
        using Val = std::atomic<float>;
    public:
        KnobMeter(Knob& _knob, const Val& _val) :
            knob(_knob),
            val(_val),
            env(0.f)
        {
            startTimerHz(static_cast<int>(PPDFPSMeters));
        }

    protected:
        Knob& knob;
        const Val& val;
        float env;

        void timerCallback() override
        {
            const auto e = val.load();
            if (env == e)
                return;
            env = e;
            knob.updateMeter(env > 1.f ? 1.f : env);
        }
    };
}

#include "configEnd.h"