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
            cutoff(u),
            q(u),
            quality(u)
        {
            makeParameter(cutoff, PID::FilterCutoff, "Cutoff");
            addAndMakeVisible(cutoff);

			makeParameter(q, PID::FilterQ, "Q");
			addAndMakeVisible(q);

			makeParameter(quality, PID::FilterSmoothUpsampler, "Quality");
			addAndMakeVisible(quality);
            
            layout.init
            (
                { 1, 13, 13, 13, 1 },
                { 1, 2, 1 }
            );
        }

        void paint(Graphics&) override
        {
        }

        void resized() override
        {
            layout.resized();

            layout.place(cutoff, 1, 1, 1, 1, false);
			layout.place(q, 2, 1, 1, 1, false);
			layout.place(quality, 3, 1, 1, 1, false);
        }

        void timerCallback() override
        {}

    protected:
        Knob cutoff, q, quality;
    };
}