#pragma once
#include "Knob.h"

namespace gui
{
    struct LowLevel :
        public Comp
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default)
        {
            

            layout.init
            (
                { 1 },
                { 1 }
            );
        }

        void paint(Graphics&) override
        {
        }

        void resized() override
        {
            
        }

    protected:
    };
}