#pragma once
#include "Param.h"
#include "Processor.h"

#include "Shared.h"
#include "Layout.h"
#include "Utils.h"

#include "Shader.h"

#include "PopUp.h"
#include "Knob.h"
#include "HighLevel.h"
#include "LowLevel.h"

#include "Tooltip.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

#include "config.h"

namespace gui
{
    using Graphics = juce::Graphics;
    using Mouse = juce::MouseEvent;
    using MouseWheel = juce::MouseWheelDetails;

    struct Editor :
        public juce::AudioProcessorEditor,
        public Timer
    {
        static constexpr int MinWidth = 100, MinHeight = 100;

        Editor(Processor& p) :
            juce::AudioProcessorEditor(p),
            audioProcessor(p),
            layout(*this),
            utils(*this, p),

            lowLevel(utils),
            highLevel(utils, &lowLevel),

            tooltip(utils, "The tooltips bar leads you to wisdom."),
            popUp(utils),

            bypassed(false),
            shadr(bypassed)
        {
            setComponentEffect(&shadr);

            setMouseCursor(makeCursor(CursorType::Default));
            
            layout.init(
                { 50, 130 },
                { 150, 10 }
            );

            addAndMakeVisible(lowLevel);
            addAndMakeVisible(highLevel);

            addAndMakeVisible(tooltip);
            addAndMakeVisible(popUp);

            startTimerHz(12);
            setOpaque(true);
            setResizable(true, true);
            {
                auto user = audioProcessor.props.getUserSettings();
                const auto w = user->getIntValue("gui/width", 620);
                const auto h = user->getIntValue("gui/height", 420);
                setSize(w, h);
            }
        }

        ~Editor()
        {
            setComponentEffect(nullptr);
        }
        
        void paint(Graphics& g) override
        {
            g.fillAll(Colours::c(gui::ColourID::Bg));
        }
        
        void resized() override
        {
            if (getWidth() < MinWidth)
                return setSize(MinWidth, getHeight());
            if (getHeight() < MinHeight)
                return setSize(getWidth(), MinHeight);

            layout.resized();

            layout.place(lowLevel, 1, 0, 1, 1, false);
            layout.place(highLevel, 0, 0, 1, 1, false);

            tooltip.setBounds(layout.bottom().toNearestInt());

            saveBounds();
        }

        void mouseEnter(const Mouse&) override
        {
            auto& evtSys = utils.getEventSystem();
            evtSys.notify(evt::Type::TooltipUpdated);
        }
        void mouseExit(const Mouse&) override
        {}
        void mouseDown(const Mouse&) override
        {}
        void mouseDrag(const Mouse&) override
        {}
        void mouseUp(const Mouse&) override
        {
            utils.getEventSystem().notify(EvtType::ClickedEmpty, this);
        }
        void mouseWheelMove(const Mouse&, const MouseWheel&) override
        {}

        Processor& audioProcessor;
    protected:
        Layout layout;
        Utils utils;

        LowLevel lowLevel;
        HighLevel highLevel;

        Tooltip tooltip;
        PopUp<6> popUp;

        bool bypassed;
        Shader shadr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

    private:
        void saveBounds()
        {
            const auto w = getWidth();
            const auto h = getHeight();
            auto user = audioProcessor.props.getUserSettings();
            user->setValue("gui/width", w);
            user->setValue("gui/height", h);
        }

        void timerCallback() override
        {
            auto b = utils.getParam(PID::Power)->getValue() < .5f;
            if (bypassed != b)
            {
                bypassed = b;
                repaintWithChildren(this);
            }
        }
    };
}

#include "configEnd.h"