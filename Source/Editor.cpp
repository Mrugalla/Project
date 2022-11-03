#include "Editor.h"
#include "gui/BGImage.h"

namespace gui
{
    Notify Editor::makeNotify(Editor& editor)
    {
        return [&e = editor](EvtType evt, const void*)
        {
            if (evt == EvtType::ColourSchemeChanged)
            {
                e.updateBgImage(true);
                e.repaint();

                e.setMouseCursor(makeCursor(CursorType::Default));
            }
        };
    }
	
    Editor::Editor(audio::Processor& p) :
        juce::AudioProcessorEditor(p),
        audioProcessor(p),
		
        layout(*this),
        utils(*this, p),

        bgImage(),
        notify(utils.getEventSystem(), makeNotify(*this)),
        imgRefresh(utils, "Click here to request a new background image."),

        tooltip(utils, "The tooltips bar leads to ultimate wisdom."),

        pluginTitle(utils, JucePlugin_Name),

        lowLevel(utils),
        tuningEditor(utils),
        highLevel(utils, &lowLevel, &tuningEditor),

        contextMenuKnobs(utils),
        contextMenuButtons(utils),

        editorKnobs(utils),

        bypassed(false),
        shadr(utils, *this)

    {
        setComponentEffect(&shadr);

        setMouseCursor(makeCursor(CursorType::Default));

        layout.init
        (
            { 2, 7 },
            { 2, 13, 1 }
        );
		
        addAndMakeVisible(tooltip);

        addAndMakeVisible(imgRefresh);
		makeSymbolButton(imgRefresh, ButtonSymbol::Img, false);
        imgRefresh.onClick.push_back([&](Button&, const Mouse&)
        {
			updateBgImage(true);
            repaint();
        });

        pluginTitle.font = getFontLobster();
        addAndMakeVisible(pluginTitle);
        pluginTitle.mode = Label::Mode::TextToLabelBounds;

        addAndMakeVisible(lowLevel);
        addAndMakeVisible(highLevel);

        addAndMakeVisible(tuningEditor);

        highLevel.init();

        addAndMakeVisible(contextMenuKnobs);
        addAndMakeVisible(contextMenuButtons);

        addChildComponent(editorKnobs);

        updateBgImage(false);

        setOpaque(true);
        setResizable(true, true);
        {
            const auto user = audioProcessor.props.getUserSettings();
            const auto w = user->getIntValue("gui/width", PPDEditorWidth);
            const auto h = user->getIntValue("gui/height", PPDEditorHeight);
            setSize(w, h);
        }
    }

    Editor::~Editor()
    {
        setComponentEffect(nullptr);
    }

    void Editor::paint(Graphics& g)
    {
        g.fillAll(Colours::c(ColourID::Bg));
        g.drawImageAt(bgImage, lowLevel.getX(), 0, false);
    }

    void Editor::resized()
    {
        if (getWidth() < MinWidth)
            return setSize(MinWidth, getHeight());
        if (getHeight() < MinHeight)
            return setSize(getWidth(), MinHeight);

        utils.resized();

        layout.resized();

        layout.place(pluginTitle, 1, 0, 1, 1, false);
        layout.place(imgRefresh, 1.9f, .5f, .1f, .5f, true);
        layout.place(lowLevel, 1, 1, 1, 1, false);
        layout.place(highLevel, 0, 0, 1, 2, false);
        
        {
            const auto bnds = lowLevel.getBounds().toFloat();

            tuningEditor.defineBounds
            (
                bnds.withX(static_cast<float>(getRight())),
				bnds
            );

            tuningEditor.updateBounds();
        }

        tooltip.setBounds(layout.bottom().toNearestInt());

        const auto thicc = utils.thicc;
        editorKnobs.setBounds(0, 0, static_cast<int>(thicc * 42.f), static_cast<int>(thicc * 12.f));

        if (bgImage.isValid())
            bgImage = bgImage.rescaled(lowLevel.getWidth(), getHeight(), Graphics::ResamplingQuality::lowResamplingQuality);
        else
            updateBgImage(true);

        saveBounds();
    }

    void Editor::mouseEnter(const Mouse&)
    {
        notify(evt::Type::TooltipUpdated);
    }

    void Editor::mouseExit(const Mouse&)
    {}

    void Editor::mouseDown(const Mouse&)
    {}

    void Editor::mouseDrag(const Mouse&)
    {}

    void Editor::mouseUp(const Mouse&)
    {
        notify(EvtType::ClickedEmpty, this);
        giveAwayKeyboardFocus();
    }

    void Editor::mouseWheelMove(const Mouse&, const MouseWheel&)
    {}

    void Editor::saveBounds()
    {
        const auto w = getWidth();
        const auto h = getHeight();
        auto user = audioProcessor.props.getUserSettings();
        user->setValue("gui/width", w);
        user->setValue("gui/height", h);
    }

    void Editor::updateBgImage(bool forced)
    {
        auto props = audioProcessor.getProps();
        if (!forced)
        {
            if (props != nullptr)
            {
                auto user = props->getUserSettings();
                if (user != nullptr)
                {
                    auto file = user->getFile();
                    file = file.getParentDirectory();
                    const auto findFiles = File::TypesOfFileToFind::findFiles;
                    const auto wildCard = "*.png";
                    for (auto f : file.findChildFiles(findFiles, true, wildCard))
                    {
                        if (f.getFileName() == "bgImage.png")
                        {
                            auto img = juce::ImageFileFormat::loadFrom(f);
                            if (img.isValid())
                            {
                                bgImage = img;
                                return;
                            }
                        }
                    }
                }
            }
        }

        auto width = lowLevel.getWidth();
		auto height = getHeight();

        if (width == 0 || height == 0)
            return;
        
        makeBGSpace(bgImage, utils.thicc, width, height);

        if (props != nullptr)
        {
            auto user = props->getUserSettings();
            if (user != nullptr)
            {
				auto file = user->getFile();
				file = file.getParentDirectory();
				file = file.getChildFile("bgImage.png");
                if (file.exists())
                    file.deleteFile();
                juce::FileOutputStream stream(file);
                juce::PNGImageFormat pngWriter;
                pngWriter.writeImageToStream(bgImage, stream);
            }
        }
    }

}