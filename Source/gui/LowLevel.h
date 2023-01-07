#pragma once
#include "Knob.h"
#include "EnvelopeGenerator.h"

namespace gui
{
    struct LowLevel :
        public Comp,
		public Timer
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default),
			Timer(),
            smooth(u)
        {
            makeParameter(smooth, PID::SmoothTest, "Smooth");
            addAndMakeVisible(smooth);
            
            layout.init
            (
                { 1, 2, 1 },
                { 1, 2, 1 }
            );
        }

        void paint(Graphics&) override
        {
        }

        void resized() override
        {
            layout.resized();

            layout.place(smooth, 1, 1, 1, 1, false);
        }

        void timerCallback() override
        {}

    protected:
        Knob smooth;
    };
}