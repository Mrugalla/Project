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
            damp(u),
            oct(u),
            semi(u),
            fine(u)
        {
            makeParameter(feedback, PID::ResonatorFeedback, "Feedback");
            makeParameter(damp, PID::ResonatorDamp, "Damp");
			makeParameter(oct, PID::ResonatorOct, "Oct");
			makeParameter(semi, PID::ResonatorSemi, "Semi");
			makeParameter(fine, PID::ResonatorFine, "Fine");

            layout.init
            (
                { 3, 5, 5, 5, 5, 5, 3 },
                { 3, 1, 5, 1, 3 }
            );

            addAndMakeVisible(feedback);
			addAndMakeVisible(damp);
			addAndMakeVisible(oct);
			addAndMakeVisible(semi);
            addAndMakeVisible(fine);
        }

        void paint(Graphics& g) override
        {
            auto thicc = utils.thicc;
            auto thicc2 = thicc * 2.f;
            const Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);

            g.setColour(Colours::c(ColourID::Txt));

            const auto resonatorArea = layout(1, 1, 5, 3);
            g.drawFittedText("Resonator", resonatorArea.toNearestInt(), Just::centredTop, 1);
            drawRectEdges(g, resonatorArea, thicc2, stroke);
        }

        void resized() override
        {
            layout.resized();

            layout.place(feedback, 1, 2, 1, 1, false);
            layout.place(damp, 2, 2, 1, 1, false);
			layout.place(oct, 3, 2, 1, 1, false);
			layout.place(semi, 4, 2, 1, 1, false);
			layout.place(fine, 5, 2, 1, 1, false);
        }

    protected:
        Knob feedback, damp, oct, semi, fine;
    };
}