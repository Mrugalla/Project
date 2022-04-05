#pragma once
#include <array>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

namespace gui
{
    using Colour = juce::Colour;
    using String = juce::String;
    using Font = juce::Font;
    using Props = juce::PropertiesFile;
    using Cursor = juce::MouseCursor;
    using Image = juce::Image;
    using Graphics = juce::Graphics;

    enum class ColourID
    {
        Bg,
        Txt,
        Abort,
        Interact,
        Inactive,
        Darken,
        Hover,
        Transp,
        Mod,
        Bias,
        NumCols
    };

    inline Colour getDefault(ColourID i) noexcept
    {
        switch (i)
        {
        case ColourID::Bg: return Colour(0xff211f28);
        case ColourID::Txt: return Colour(0xff75da10);
        case ColourID::Inactive: return Colour(0xff808080);
        case ColourID::Abort: return Colour(0xffff0000);
        case ColourID::Interact: return Colour(0xff00ffc5);
        case ColourID::Darken: return Colour(0xea000000);
        case ColourID::Hover: return Colour(0x827965cb);
        case ColourID::Mod: return Colour(0xffac6700);
        case ColourID::Bias: return Colour(0xfff3db00);
        default: return Colour(0x00000000);
        }
    }
    inline String toString(ColourID i)
    {
        switch (i)
        {
        case ColourID::Bg: return "background";
        case ColourID::Txt: return "text";
        case ColourID::Abort: return "abort";
        case ColourID::Interact: return "interact";
        case ColourID::Inactive: return "inactive";
        case ColourID::Darken: return "darken";
        case ColourID::Hover: return "hover";
        case ColourID::Transp: return "transp";
        case ColourID::Mod: return "mod";
        case ColourID::Bias: return "bias";
        default: return "";
        }
    }

    inline juce::String toStringProps(ColourID i) { return "colour" + toString(i); }

    class Colours
    {
        using Array = std::array<Colour, static_cast<int>(ColourID::NumCols)>;
    public:
        Colours() :
            cols(),
            props(nullptr)
        {}

        void init(Props* p)
        {
            props = p;
            if (props->isValidFile())
                for (auto i = 0; i < static_cast<int>(ColourID::NumCols); ++i)
                {
                    const auto cID = static_cast<ColourID>(i);
                    const auto colStr = props->getValue(toStringProps(cID), getDefault(cID).toString());
                    set(i, juce::Colour::fromString(colStr));
                }
        }

        bool set(const String& i, Colour col)
        {
            for (auto j = 0; j < cols.size(); ++j)
                if (i == cols[j].toString())
                    return set(j, col);
            return false;
        }
        bool set(ColourID i, Colour col) noexcept
        {
            return set(static_cast<int>(i), col);
        }
        bool set(int i, Colour col) noexcept
        {
            if (props->isValidFile())
            {
                cols[i] = col;
                props->setValue(toStringProps(ColourID(i)), col.toString());
                if (props->needsToBeSaved())
                {
                    props->save();
                    props->sendChangeMessage();
                    return true;
                }
            }
            return false;
        }
        Colour operator()(ColourID i) const noexcept
        {
            return get(static_cast<int>(i));
        }
        Colour operator()(int i) const noexcept
        {
            return get(i);
        }
        Colour get(int i) const noexcept
        {
            return cols[i];
        }

        static Colours c;
    protected:
        Array cols;
        Props* props;
    };

    inline Font getFont(const char* ttf, size_t size)
    {
        return juce::Font(juce::Typeface::createSystemTypefaceFor(ttf, size));
    }
    inline Font getFontNEL()
    {
        return getFont(BinaryData::nel19_ttf, BinaryData::nel19_ttfSize);
    }
}