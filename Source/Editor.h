#pragma once
#include "Param.h"
#include "Processor.h"

#include "Shared.h"
#include "Layout.h"
#include "Utils.h"

#include "Shader.h"

#include "Knob.h"
#include "HighLevel.h"
#include "Tooltip.h"

#include "ValueBubble.h"

#include <juce_gui_basics/juce_gui_basics.h>

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

            highLevel(utils),

            tooltip(utils, "The tooltips bar leads you to wisdom."),

            valueBubble(utils),
            
            bypassed(false),
            shadr(bypassed)
        {
            setComponentEffect(&shadr);

            setMouseCursor(makeCursor(CursorType::Default));
            
            layout.init(
                { 50, 130 },
                { 150, 10 }
            );

            addAndMakeVisible(highLevel);

            addAndMakeVisible(tooltip);

            addChildComponent(valueBubble);

            startTimerHz(12);
            setOpaque(true);
            setResizable(true, true);
            {
                auto w = 620;
                auto h = 420;
                auto var = p.state.get("gui", "width");
                if (var)
                    w = static_cast<int>(*var);
                var = p.state.get("gui", "height");
                if (var)
                    h = static_cast<int>(*var);
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

            layout.place(highLevel, 0, 0, 1, 1, false);

            tooltip.setBounds(layout.bottom().toNearestInt());

            {
                const auto w = ValueBubbleSizePercentage * getWidth() / 100;
                const auto h = ValueBubbleSizePercentage * getHeight() / 100;
                valueBubble.setSize(w, h);
            }

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
        {}
        void mouseWheelMove(const Mouse&, const MouseWheel&) override
        {}

        Processor& audioProcessor;
    protected:
        Layout layout;
        Utils utils;

        HighLevel highLevel;

        Tooltip tooltip;

        ValueBubble valueBubble;

        bool bypassed;
        Shader shadr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

    private:
        void saveBounds()
        {
            const auto w = getWidth();
            const auto h = getHeight();
            audioProcessor.state.set("gui", "width", w, false);
            audioProcessor.state.set("gui", "height", h, false);
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