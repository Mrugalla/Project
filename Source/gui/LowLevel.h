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
            feedback(u),
            damp(u)
        {
            makeParameter(feedback, PID::ResonatorFeedback, "Feedback");
            makeParameter(damp, PID::ResonatorDamp, "Damp");

            layout.init
            (
                { 3, 3, 3, 3 },
                { 3, 3, 3 }
            );

            addAndMakeVisible(feedback);
			addAndMakeVisible(damp);
        }

    protected:
        Knob feedback, damp;
        
        void paint(Graphics&) override {}

        void resized() override
        {
            layout.resized();

            layout.place(feedback, 1, 1, 1, 1, false);
			layout.place(damp, 2, 1, 1, 1, false);
        }
    };
}