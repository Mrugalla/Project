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
            freq(u)
        {
            makeParameter(freq, PID::RingModFreq, "Freq");
			
            layout.init(
                { 1, 3, 3, 3, 1 },
                { 1, 3, 1 }
            );

            addAndMakeVisible(freq);
        }

    protected:
        Knob freq;
        
        void paint(Graphics&) override {}

        void resized() override
        {
            layout.resized();

            layout.place(freq, 2, 1, 1, 1, false);
        }
    };
}