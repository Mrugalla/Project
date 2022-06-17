#pragma once
#include "Knob.h"
#include "SplineEditor.h"

namespace gui
{
    struct LowLevel :
        public Comp
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default),
            spline(u, "Dis tha spline editor bro"),
            crushGain(u, "Crush Gain", PID::CrushGain),
            dummyParam(u, "Dummy Param", PID::AnotherDummyParam)
        {
            
            layout.init(
                { 1, 2, 2, 1 },
                { 1, 13, 2, 1 }
            );

            addAndMakeVisible(spline);
            addAndMakeVisible(crushGain);
            addAndMakeVisible(dummyParam);
        }

    protected:
        SplineEditor spline;
        Knob crushGain, dummyParam;
        
        void paint(Graphics&) override {}

        void resized() override
        {
            layout.resized();

            layout.place(spline, 1, 1, 2, 1, false);
            layout.place(crushGain, 1, 2, 1, 1, false);
            layout.place(dummyParam, 2, 2, 1, 1, false);
        }
    };
}