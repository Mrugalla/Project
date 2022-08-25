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
            feedback(u)
        {
            makeParameter(feedback, PID::ResonatorFeedback, "Feedback");

            layout.init(
                { 1, 3, 1 },
                { 1, 3, 1 }
            );

            addAndMakeVisible(feedback);
        }

    protected:
        Knob feedback;
        
        void paint(Graphics&) override {}

        void resized() override
        {
            layout.resized();

            layout.place(feedback, 1, 1, 1, 1, false);
        }
    };
}