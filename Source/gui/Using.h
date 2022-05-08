#pragma once

#include "BinaryData.h"

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Processor.h"
#include "../param/Param.h"

namespace gui
{
    using Colour = juce::Colour;
    using String = juce::String;
    using Font = juce::Font;
    using Props = juce::PropertiesFile;
    using AppProps = juce::ApplicationProperties;
    using Cursor = juce::MouseCursor;
    using Image = juce::Image;
    using Graphics = juce::Graphics;
    using Mouse = juce::MouseEvent;
    using Graphics = juce::Graphics;
    using Just = juce::Justification;
    using Timer = juce::Timer;
    using Path = juce::Path;
    using Point = juce::Point<int>;
    using PointF = juce::Point<float>;
    using Bounds = juce::Rectangle<int>;
    using BoundsF = juce::Rectangle<float>;
    using Line = juce::Line<int>;
    using LineF = juce::Line<float>;
    using Image = juce::Image;

    using Component = juce::Component;
    using Processor = audio::Processor;

    using PID = param::PID;
    using Param = param::Param;
    using Params = param::Params;

    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = Tau * .5f;;
    static constexpr float PiHalf = Tau * .25f;
    static constexpr float PiQuart = Tau * .125f;
}