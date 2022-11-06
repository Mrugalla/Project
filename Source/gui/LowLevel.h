#pragma once
#include "Knob.h"

namespace gui
{
    struct LowLevel :
        public Comp
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default),
            atk(u),
			dcy(u),
			sus(u),
			rls(u),
			atkShape(u),
			dcyShape(u),
			rlsShape(u)
        {
            makeParameter(atk, PID::EnvGenAttack, "Attack", true);
            addAndMakeVisible(atk);

			makeParameter(dcy, PID::EnvGenDecay, "Decay", true);
			addAndMakeVisible(dcy);
			
			makeParameter(sus, PID::EnvGenSustain, "Sustain", true);
			addAndMakeVisible(sus);

			makeParameter(rls, PID::EnvGenRelease, "Release", true);
			addAndMakeVisible(rls);

			makeParameter(atkShape, PID::EnvGenAtkShape, "Shape", true);
			addAndMakeVisible(atkShape);

			makeParameter(dcyShape, PID::EnvGenDcyShape, "Shape", true);
			addAndMakeVisible(dcyShape);

			makeParameter(rlsShape, PID::EnvGenRlsShape, "Shape", true);
			addAndMakeVisible(rlsShape);

            layout.init
            (
                { 1, 1, 1, 1, 1, 1 },
                { 1, 1 }
            );
        }

        void paint(Graphics&) override
        {
        }

        void resized() override
        {
            layout.resized();

            layout.place(atk, 1, 0, 1, 1);
			layout.place(dcy, 2, 0, 1, 1);
			layout.place(sus, 3, 0, 1, 1);
			layout.place(rls, 4, 0, 1, 1);

			layout.place(atkShape, 1, 1, 1, 1);
			layout.place(dcyShape, 2, 1, 1, 1);
			layout.place(rlsShape, 4, 1, 1, 1);
        }

    protected:
        Knob atk, dcy, sus, rls;
        Knob atkShape, dcyShape, rlsShape;
    };
}