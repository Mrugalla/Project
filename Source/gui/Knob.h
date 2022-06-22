#pragma once
#include "GUIParams.h"
#include <functional>

namespace gui
{
    struct Knob :
        public Parametr,
        public Timer
    {
        static constexpr float AngleWidth = PiQuart * 3.f;
        static constexpr float AngleRange = AngleWidth * 2.f;

        Knob(Utils&, String&& /*name*/, PID, bool /*modulatable*/ = true);

        void updateMeter(float);

        void setCID(ColourID);

        Label& getLabel() noexcept;

    protected:
        BoundsF knobBounds;
        Label label;
        float dragY, valMeter;
        ColourID cID;

        void timerCallback() override;

        void paint(juce::Graphics&) override;

        void resized() override;

        void mouseEnter(const Mouse&) override;

        void mouseExit(const Mouse&) override;

        void mouseDown(const Mouse&) override;

        void mouseDrag(const Mouse&) override;

        void mouseUp(const Mouse&) override;

        void mouseWheelMove(const Mouse&, const MouseWheel&) override;

    };

    class KnobMeter :
        public Timer
    {
        using Val = std::atomic<float>;
    public:
        KnobMeter(Knob&, const Val&);

    protected:
        Knob& knob;
        const Val& val;
        float env;

        void timerCallback() override;
    };



    struct Knob2 :
        public Comp,
        public Timer
    {
        using Func = std::function<void(Knob2&)>;
        using OnDrag = std::function<void(Knob2&, PointF&)>;
        using OnUp = std::function<void(Knob2&, const Mouse&)>;
        using OnTimer = std::function<bool(Knob2&)>;
        using OnPaint = std::function<void(Knob2&, Graphics&)>;

        Knob2(Utils& u, const String& _name = "", const String& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
            Comp(u, _tooltip, _cursorType),
            onEnter([](Knob2&){}),
            onExit([](Knob2&){}),
            onDown([](Knob2&){}),
            onWheel([](Knob2&){}),
            onResize([](Knob2&){}),
            onDrag([](Knob2&, PointF&){}),
            onUp([](Knob2&, const Mouse&){}),
            onTimer([](Knob2&) { return false; }),
            onPaint([](Knob2&, Graphics&){}),
            label(u, _name),
            dragXY(),
            knobBounds(),
            values(),
            comps(),
            states(),
            hidesCursor(true),
            locked(false),
            activeCursor(_cursorType)
        {
            setName(_name);

            label.textCID = ColourID::Txt;
            label.just = Just::centred;
            label.mode = Label::Mode::TextToLabelBounds;

            addAndMakeVisible(label);
        }

        void init(std::vector<int>&& distX, std::vector<int>&& distY)
        {
            layout.init(
                distX,
                distY
            );
        }

        void timerCallback() override
        {
            if (onTimer(*this))
                repaint();
        }

        void paint(juce::Graphics& g) override
        {
            onPaint(*this, g);
        }

        void resized() override
        {
            layout.resized();
            onResize(*this);
        }

        void mouseEnter(const Mouse& mouse)
        {
            Comp::mouseEnter(mouse);
            onEnter(*this);
        }

        void mouseExit(const Mouse&)
        {
            onExit(*this);
        }

        void mouseDown(const Mouse& mouse)
        {
            if (mouse.mods.isLeftButtonDown())
            {
                if(hidesCursor)
                    hideCursor();

                dragXY.setXY(
                    mouse.position.x,
                    mouse.position.y
                );

                onDown(*this);
            }
        }

        void mouseDrag(const Mouse& mouse)
        {
            if (mouse.mods.isLeftButtonDown())
            {
                auto dragOffset = mouse.position - dragXY;
                if (mouse.mods.isShiftDown())
                    dragOffset *= SensitiveDrag;
                onDrag(*this, dragOffset);
                dragXY = mouse.position;
            }
        }

        void mouseUp(const Mouse& mouse)
        {
            onUp(*this, mouse);

            if(hidesCursor)
                showCursor(*this);
        }

        void mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
        {
            if (mouse.mods.isAnyMouseButtonDown())
                return;

            const bool reversed = wheel.isReversed ? -1.f : 1.f;
            const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
            if (isTrackPad)
                dragXY.setXY(
                    reversed * wheel.deltaX,
                    reversed * wheel.deltaY
                    );
            else
            {
                const auto deltaXPos = wheel.deltaX > 0.f ? 1.f : -1.f;
                const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
                dragXY.setXY(
                    reversed * WheelDefaultSpeed * deltaXPos,
                    reversed * WheelDefaultSpeed * deltaYPos
                );
            }

            if (mouse.mods.isShiftDown())
                dragXY *= SensitiveDrag;
            
            onWheel(*this);
        }

        void setLocked(bool lckd)
        {
            locked = lckd;
            if (locked)
            {
                setCursorType(CursorType::Inactive);
                setAlpha(.2f);
            }
            else
            {
                setCursorType(activeCursor);
                setAlpha(1.f);
            }
        }

        Func onEnter, onExit, onDown, onWheel, onResize;
        OnDrag onDrag;
        OnUp onUp;
        OnTimer onTimer;
        OnPaint onPaint;
        Label label;
        PointF dragXY;
        BoundsF knobBounds;
        std::vector<float> values;
        std::vector<std::unique_ptr<Comp>> comps;
        std::vector<int> states;
        bool hidesCursor, locked;
        CursorType activeCursor;
    };

    inline void makeParameter(Knob2& knob, PID pID, const String& name, bool modulatable)
    {
        knob.setName(name);
        knob.label.setText(name);
        knob.setTooltip(param::toTooltip(pID));

        auto& utils = knob.getUtils();
        auto param = utils.getParam(pID);
        const auto angleWidth = PiQuart * 3.f;
        const auto angleRange = angleWidth * 2.f;

        knob.locked = param->isLocked();

        knob.onEnter = [param](Knob2& k)
        {
            k.label.setText(param->getCurrentValueAsText());
            k.label.repaint();
        };
        
        knob.onExit = [](Knob2& k)
        {
            k.label.setText(k.getName());
            k.label.repaint();
        };

        knob.onDown = [param](Knob2& k)
        {
            if (param->isInGesture())
                return;
            param->beginGesture();

            k.label.setText(param->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onDrag = [param](Knob2& k, const PointF& dragOffset)
        {
            const auto speed = 1.f / k.getUtils().getDragSpeed();

            const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() - dragOffset.y * speed);
            param->setValueNotifyingHost(newValue);

            k.label.setText(param->getCurrentValueAsText());
            k.label.repaint();

            k.notify(EvtType::ParametrDragged, &k);
        };

        knob.onUp = [param, angleWidth, angleRange](Knob2& k, const Mouse& mouse)
        {
            if (mouse.mods.isLeftButtonDown())
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                {
                    if (mouse.mods.isCtrlDown())
                        param->setValueNotifyingHost(param->getDefaultValue());
                    else
                    {
                        PointF centre(
                            static_cast<float>(k.getWidth()) * .5f,
                            static_cast<float>(k.getHeight()) * .5f
                        );
                        const LineF fromCentre(centre, mouse.position);
                        const auto angle = fromCentre.getAngle();

                        const auto newValue = juce::jlimit(0.f, 1.f, (angle + angleWidth) / angleRange);
                        param->setValue(newValue);
                    }
                }
                param->endGesture();
            }
            else if (mouse.mods.isRightButtonDown())
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isCtrlDown())
                        param->setValueWithGesture(param->getDefaultValue());
                    else
                        k.notify(EvtType::ParametrRightClicked, &k);
            
            k.label.setText(param->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onWheel = [param](Knob2& k)
        {
            const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + k.dragXY.y);
            param->setValueWithGesture(newValue);

            k.label.setText(param->getCurrentValueAsText());
            k.label.repaint();
        };

        enum { Value, MaxModDepth, ValMod, ModBias, NumValues };
        knob.values.reserve(NumValues);
        auto& values = knob.values;

        values.emplace_back(param->getValue());
        values.emplace_back(param->getMaxModDepth());
        values.emplace_back(param->getValMod());
        values.emplace_back(param->getModBias());

        knob.onTimer = [param](Knob2& k)
        {
            const auto lckd = param->isLocked();
            if (k.locked != lckd)
                k.locked = lckd;

            const auto vn = param->getValue();
            const auto mmd = param->getMaxModDepth();
            const auto vm = param->getValMod();
            const auto mb = param->getModBias();

            auto& vals = k.values;

            if (vals[Value] != vn || vals[MaxModDepth] != mmd || vals[ValMod] != vm || vals[ModBias] != mb)
            {
                vals[Value] = vn;
                vals[MaxModDepth] = mmd;
                vals[ValMod] = vm;
                vals[ModBias] = mb;
                return true;
            }

            return false;
        };

        enum { ModDial, NumComps };
        auto modDial = std::make_unique<Knob2>(knob.getUtils(), "M", "Drag this to modulate the macro's modulation depth.", CursorType::Mod);
        {
            auto& dial = *modDial;
            
            auto& label = dial.label;
            label.mode = Label::Mode::TextToLabelBounds;
            label.textCID = ColourID::Bg;

            enum { StateMaxModDepth, StateModBias, NumStates };
            dial.states.push_back(StateMaxModDepth);

            dial.onResize = [](Knob2& k)
            {
                k.knobBounds = k.getLocalBounds().toFloat();
                const auto thicc = k.getUtils().thicc * .5f;
                k.label.setBounds(k.knobBounds.reduced(thicc).toNearestInt());
            };

            dial.onPaint = [](Knob2& k, Graphics& g)
            {
                auto state = k.states[0];

                Colour col;
                switch (state)
                {
                case StateMaxModDepth:
                    col = Colours::c(ColourID::Mod);
                    break;
                case StateModBias:
                    col = Colours::c(ColourID::Bias);
                    break;
                }
                g.setColour(col);
                g.fillEllipse(k.knobBounds);
            };

            dial.onDrag = [param](Knob2& k, PointF& dragOffset)
            {
                auto state = k.states[0];
                auto& utils = k.getUtils();
                if (state == StateModBias)
                    dragOffset *= 2.f;
                const auto speed = 1.f / utils.getDragSpeed();
                dragOffset *= speed;

                float newValue;
                switch (state)
                {
                case StateMaxModDepth:
                    newValue = param->getMaxModDepth() - dragOffset.y;
                    param->setMaxModDepth(newValue);
                    break;
                case StateModBias:
                    newValue = param->getModBias() - dragOffset.y;
                    param->setModBias(newValue);
                    break;
                }
            };

            dial.onUp = [param](Knob2& k, const Mouse& mouse)
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                {
                    auto state = k.states[0];

                    if (mouse.mods.isCtrlDown())
                    {
                        switch (state)
                        {
                        case StateMaxModDepth:
                            param->setMaxModDepth(0.f);
                            break;
                        case StateModBias:
                            param->setModBias(.5f);
                            break;
                        }
                    }
                    else if (mouse.mods.isRightButtonDown())
                    {
                        state = (state + 1) % NumStates;

                        switch (state)
                        {
                        case StateMaxModDepth:
                            k.label.setText("M");
                            k.setCursorType(CursorType::Mod);
                            k.activeCursor = CursorType::Mod;
                            break;
                        case StateModBias:
                            k.label.setText("B");
                            k.setCursorType(CursorType::Bias);
                            k.activeCursor = CursorType::Bias;
                            break;
                        }

                        repaintWithChildren(&k);
                    }
                }
            };

            knob.comps.push_back(std::move(modDial));
            knob.addAndMakeVisible(*knob.comps.back());
        }

        knob.onResize = [](Knob2& k)
        {
            const auto thicc = k.getUtils().thicc;
            auto& layout = k.getLayout();

            k.knobBounds = layout(0, 0, 3, 2, true).reduced(thicc);
            layout.place(k.label, 0, 2, 3, 1, false);
            layout.place(*k.comps[ModDial], 1, 1, 1, 1, true);
            
        };

        knob.onPaint = [angleWidth, angleRange, modulatable](Knob2& k, Graphics& g)
        {
            const auto& vals = k.values;
            const auto thicc = k.getUtils().thicc;
            const auto thicc2 = thicc * 2.f;
            const auto thicc3 = thicc * 3.f;
            Stroke strokeType(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
            const auto radius = k.knobBounds.getWidth() * .5f;
            const auto radiusBetween = radius - thicc;
            const auto radiusInner = radius - thicc2;
            PointF centre(
                radius + k.knobBounds.getX(),
                radius + k.knobBounds.getY()
            );

            const auto col = Colours::c(ColourID::Interact);

            /*
            if (valMeter != 0.f)
            {
                g.setColour(Colours::c(ColourID::Txt));
                Path meterArc;

                const auto meterAngle = angleRange * valMeter - angleWidth;

                meterArc.addCentredArc(
                    centre.x, centre.y,
                    radiusBetween, radiusBetween,
                    0.f,
                    -angleWidth, meterAngle,
                    true
                );

                strokeType.setStrokeThickness(thicc2);
                g.strokePath(meterArc, strokeType);
                strokeType.setStrokeThickness(thicc);
            }
            */

            //draw outlines
            {
                g.setColour(col);
                Path outtaArc;

                outtaArc.addCentredArc(
                    centre.x, centre.y,
                    radius, radius,
                    0.f,
                    -angleWidth, angleWidth,
                    true
                );
                outtaArc.addCentredArc(
                    centre.x, centre.y,
                    radiusInner, radiusInner,
                    0.f,
                    -angleWidth, angleWidth,
                    true
                );

                g.strokePath(outtaArc, strokeType);
            }

            const auto valNormAngle = vals[Value] * angleRange;
            const auto valAngle = -angleWidth + valNormAngle;
            const auto radiusExt = radius + thicc;

            // draw modulation
            if (modulatable)
            {
                const auto valModAngle = vals[ValMod] * angleRange;
                const auto modAngle = -angleWidth + valModAngle;
                const auto modTick = LineF::fromStartAndAngle(centre, radiusExt, modAngle);

                g.setColour(Colours::c(ColourID::Bg));
                g.drawLine(modTick, thicc * 4.f);

                const auto maxModDepthAngle = juce::jlimit(-angleWidth, angleWidth, valNormAngle + vals[MaxModDepth] * angleRange - angleWidth);
                const auto biasAngle = angleRange * vals[ModBias] - angleWidth;

                g.setColour(Colours::c(ColourID::Bias));
                {
                    Path biasPath;
                    biasPath.addCentredArc(
                        centre.x, centre.y,
                        radiusInner, radiusInner,
                        0.f,
                        0.f, biasAngle,
                        true
                    );
                    g.strokePath(biasPath, strokeType);
                }

                g.setColour(Colours::c(ColourID::Mod));
                g.drawLine(modTick.withShortenedStart(radiusInner), thicc2);
                {
                    Path modPath;
                    modPath.addCentredArc(
                        centre.x, centre.y,
                        radius, radius,
                        0.f,
                        maxModDepthAngle, valAngle,
                        true
                    );
                    g.strokePath(modPath, strokeType);
                }
            }
            // draw tick
            {
                const auto tickLine = LineF::fromStartAndAngle(centre, radiusExt, valAngle);
                g.setColour(Colours::c(ColourID::Bg));
                g.drawLine(tickLine, thicc3);
                g.setColour(col);
                g.drawLine(tickLine.withShortenedStart(radiusInner), thicc2);
            }
        };

        knob.init(
            { 40, 40, 40 },
            { 100, 40, 40 }
        );

        knob.startTimerHz(PPDFPSKnobs);
    }
}