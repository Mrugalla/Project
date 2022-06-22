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
            crushGain(u),
            dummyParam(u)
        {
            makeParameter(crushGain, PID::CrushGain, "Crush Gain", true);
            makeParameter(dummyParam, PID::AnotherDummyParam, "Dummy Param", true);

            layout.init(
                { 1, 2, 2, 1 },
                { 1, 5, 2, 3, 1 }
            );

            addAndMakeVisible(spline);
            addAndMakeVisible(crushGain);
            addAndMakeVisible(dummyParam);
        }

    protected:
        SplineEditorPanel spline;
        Knob2 crushGain, dummyParam;
        
        void paint(Graphics&) override {}

        void resized() override
        {
            layout.resized();

            layout.place(spline, 1, 1, 2, 1, false);
            layout.place(crushGain, 1, 3, 1, 1, false);
            layout.place(dummyParam, 2, 3, 1, 1, false);
        }
    };
}