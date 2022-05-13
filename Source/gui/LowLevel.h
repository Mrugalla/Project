#pragma once
#include "Knob.h"
#include "Shader.h"
#include "config.h"

namespace gui
{
    struct LowLevel :
        public Comp
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default),
            crushGain(u, "Crush Gain", PID::CrushGain),
            dummyParam(u, "Dummy Param", PID::AnotherDummyParam)
        {
            
            layout.init(
                { 30, 10, 10, 30 },
                { 30, 10, 10, 30 }
            );

            addAndMakeVisible(crushGain);
            addAndMakeVisible(dummyParam);
        }

    protected:
        Knob crushGain, dummyParam;
        
        void paint(Graphics&) override {}

        void resized() override
        {
            layout.resized();

            layout.place(crushGain, 1, 1, 1, 1, false);
            layout.place(dummyParam, 2, 1, 1, 1, false);
        }
    };
}

#include "configEnd.h"