#pragma once
#include "Label.h"

namespace gui
{
    static constexpr float SensitiveDrag = .2f;
    static constexpr float WheelDefaultSpeed = .02f;
    static constexpr float WheelInertia = .9f;

    static constexpr float LockAlpha = .4f;

    struct Parametr :
        public Comp
    {
        struct ModDial :
            public Comp
        {
            enum class State { MaxModDepth, Bias, NumStates };
            static constexpr int NumStates = static_cast<int>(State::NumStates);

            ModDial(Utils& u, Parametr& _parametr) :
                Comp(u, "Drag this to modulate the macro's modulation depth.", CursorType::Mod),
                parametr(_parametr),
                label(u, "M"),
                dragY(0.f),
                state(State::MaxModDepth)
            {
                label.textCID = ColourID::Bg;
                addAndMakeVisible(label);
            }

            void paint(Graphics& g) override
            {
                const auto bounds = getLocalBounds().toFloat();
                switch (state)
                {
                case State::MaxModDepth:
                    g.setColour(Colours::c(ColourID::Mod));
                    break;
                case State::Bias:
                    g.setColour(Colours::c(ColourID::Bias));
                    break;
                }
                g.fillEllipse(bounds);
            }

            State getState() const noexcept { return state; }
        protected:
            Parametr& parametr;
            Label label;
            float dragY;
            State state;

            void resized() override
            {
                const auto thicc = utils.thicc();
                label.setBounds(getLocalBounds().toFloat().reduced(thicc * .5f).toNearestInt());
            }

            void mouseDown(const Mouse& mouse) override
            {
                switch (state)
                {
                case State::MaxModDepth:
                    dragY = mouse.position.y / utils.getDragSpeed();
                case State::Bias:
                    dragY = mouse.position.y / utils.getDragSpeed() * 2.f;
                    break;
                }
            }
            void mouseDrag(const Mouse& mouse) override
            {
                if (mouse.mods.isLeftButtonDown())
                {
                    float dragYNew;
                    switch (state)
                    {
                    case State::MaxModDepth:
                        dragYNew = mouse.position.y / utils.getDragSpeed();
                    case State::Bias:
                        dragYNew = mouse.position.y / utils.getDragSpeed() * 2.f;
                        break;
                    }
                    auto dragOffset = dragYNew - dragY;
                    if (mouse.mods.isShiftDown())
                        dragOffset *= SensitiveDrag;
                    float newValue;
                    switch (state)
                    {
                    case State::MaxModDepth:
                        newValue = parametr.param.getMaxModDepth() - dragOffset;
                        parametr.param.setMaxModDepth(newValue);
                        break;
                    case State::Bias:
                        newValue = parametr.param.getModBias() - dragOffset;
                        parametr.param.setModBias(newValue);
                        break;
                    }
                    
                    dragY = dragYNew;
                }
            }
            void mouseUp(const Mouse& mouse) override
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isCtrlDown())
                        switch (state)
                        {
                        case State::MaxModDepth:
                            parametr.param.setMaxModDepth(0.f);
                            break;
                        case State::Bias:
                            parametr.param.setModBias(.5f);
                            break;
                        }
                    else if (mouse.mods.isRightButtonDown())
                    {
                        state = static_cast<State>((static_cast<int>(state) + 1) % NumStates);
                        switch (state)
                        {
                        case State::MaxModDepth:
                            label.setText("M");
                            break;
                        case State::Bias:
                            label.setText("B");
                            break;
                        }
                        repaintWithChildren(&parametr);
                    }
            }
        };

        Parametr(Utils& u, PID _pID, bool _modulatable) :
            Comp(u, param::toTooltip(_pID)),
            param(*u.getParam(_pID)),
            modDial(u, *this),
            valNorm(param.getValue()),
            maxModDepth(param.getMaxModDepth()),
            valMod(param.getValMod()),
            modBias(param.getModBias()),
            locked(param.isLocked()),
            modulatable(_modulatable)
        {
            if(modulatable)
                addAndMakeVisible(modDial);

            setLocked(param.isLocked());
        }

        PID getPID() const noexcept { return param.id; }

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
                setCursorType(CursorType::Interact);
                setAlpha(1.f);
            }
        }
    protected:
        Param& param;
        ModDial modDial;
        float valNorm, maxModDepth, valMod, modBias;
        bool locked;
        const bool modulatable;
    };
}