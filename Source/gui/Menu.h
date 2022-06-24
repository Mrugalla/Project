#pragma once
#include "TextEditor.h"

namespace gui
{
	Just getJust(const String&);

	// not used yet:
	struct ColourShifter :
		public Comp
	{
		enum
		{
			Display,
			Hue,
			Sat,
			Bright,
			NumControls
		};

		ColourShifter(Utils& u) :
			Comp(u, "Click and/or drag on this component to select a colour."),
			bounds(),
			label
			{
				Label(u, "Display"),
				Label(u, "Hue"),
				Label(u, "Saturation"),
				Label(u, "Brightness")
			},
			currentColour(0xffff0000)
		{
			layout.init
			(
				{ 8, 1 },
				{ 1, 1, 1, 1 }
			);
		}

		void setCurrentColour(Colour c)
		{
			currentColour = c;
			repaint();
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc;
			g.fillRoundedRectangle(label[Display].getBounds().toFloat(), thicc);

			float hsb[4]
			{
				0.f,
				currentColour.getHue(),
				currentColour.getSaturation(),
				currentColour.getBrightness()
			};

			{
				const auto w = static_cast<float>(bounds[Hue].getWidth());
				const auto h = static_cast<float>(bounds[Hue].getHeight());

				auto hue = 0.f;
				const auto inc = 1.f / w;

				for (auto x = 0.f; x < w; ++x, hue += inc)
				{
					const auto col = Colour::fromHSL(hue, hsb[Sat], hsb[Bright], 1.f);
					g.setColour(col);
					g.drawRect(x, 0.f, 1.f, h);
				}

				g.setColour(Colour(0xff000000));
				const auto x = inc * hsb[Hue];
				g.drawRect(x - 1.f, 0.f, 3.f, h);
			}
			{
				const auto w = static_cast<float>(bounds[Sat].getWidth());
				const auto h = static_cast<float>(bounds[Sat].getHeight());

				auto sat = 0.f;
				const auto inc = 1.f / w;

				for (auto x = 0.f; x < w; ++x, sat += inc)
				{
					const auto col = Colour::fromHSL(hsb[Hue], sat, hsb[Bright], 1.f);
					g.setColour(col);
					g.drawRect(x, 0.f, 1.f, h);
				}

				g.setColour(Colour(0xff000000));
				const auto x = inc * hsb[Sat];
				g.drawRect(x - 1.f, 0.f, 3.f, h);
			}
			{
				const auto w = static_cast<float>(bounds[Bright].getWidth());
				const auto h = static_cast<float>(bounds[Bright].getHeight());

				auto bright = 0.f;
				const auto inc = 1.f / w;

				for (auto x = 0.f; x < w; ++x, bright += inc)
				{
					const auto col = Colour::fromHSL(hsb[Hue], hsb[Sat], bright, 1.f);
					g.setColour(col);
					g.drawRect(x, 0.f, 1.f, h);
				}

				g.setColour(Colour(0xff000000));
				const auto x = inc * hsb[Bright];
				g.drawRect(x - 1.f, 0.f, 3.f, h);
			}
		}

		void resized() override
		{
			layout.resized();

			layout.place(label[Display], 0, 0, 2, 1, false);

			for (auto i = 1; i < NumControls; ++i)
			{
				bounds[i] = layout(0, i, 1, 1, false);
				layout.place(label[i], 1, i, 1, 1, false);
			}
		}

		void mouseDown(const Mouse&) override
		{

		}
		void mouseDrag(const Mouse&) override
		{

		}
		void mouseUp(const Mouse&) override
		{

		}
	protected:
		std::array<BoundsF, NumControls> bounds;
		std::array<Label, NumControls> label;
		Colour currentColour;
	};

	struct ColourSelector :
		public Comp,
		public Timer
	{
		using CS = juce::ColourSelector;

		ColourSelector(Utils&);

		void paint(Graphics&) override;

		void resized() override;

		void timerCallback() override;

	protected:
		CS selector;
		Button revert, deflt;
		Colour curSheme;
	};

	///

	struct ErkenntnisseComp :
		public Comp,
		public Timer
	{
		ErkenntnisseComp(Utils& u) :
			Comp(u, "", CursorType::Default),
			Timer(),
			editor(u, "Enter or edit wisdom.", "Enter wisdom..."),
			date(u, ""),
			manifest(u, "Click here to manifest wisdom to the manifest of wisdom!"),
			inspire(u, "Click here to get inspired by past wisdom of the manifest of wisdom!"),
			revealFolder(u, "Click here to reveal the manifest of wisdom!")
		{
			const File folder(getFolder());
			if (!folder.exists())
				folder.createDirectory();

			layout.init(
				{ 1, 1, 1 },
				{ 8, 1, 1 }
			);

			addAndMakeVisible(editor);
			addAndMakeVisible(date);
			date.mode = Label::Mode::TextToLabelBounds;

			addAndMakeVisible(manifest);
			addAndMakeVisible(inspire);
			addAndMakeVisible(revealFolder);

			makeTextButton(manifest, "Manifest");
			makeTextButton(inspire, "Inspire");
			makeTextButton(revealFolder, "Reveal\nFolder");

			editor.onReturn = [&]()
			{
				saveToDisk();
			};

			manifest.onClick.push_back([&](Button&)
			{
				saveToDisk();
			});

			inspire.onClick.push_back([&](Button&)
			{
				const File folder(getFolder());
				
				const auto fileTypes = File::TypesOfFileToFind::findFiles;
				const String extension(".txt");
				const auto wildCard = "*" + extension;
				const auto numFiles = folder.getNumberOfChildFiles(fileTypes, wildCard);
				Random rand;
				auto idx = rand.nextInt(numFiles);

				const RangedDirectoryIterator files(
					folder,
					false,
					wildCard,
					fileTypes
				);

				for (const auto& it : files)
				{
					if (idx == 0)
					{
						const File file(it.getFile());
						parse(file.getFileName());
						editor.setText(file.loadFileAsString());
						editor.repaint();
						return;
					}
					else
						--idx;
				}
			});

			revealFolder.onClick.push_back([&](Button&)
			{
				const File folder(getFolder());
				folder.revealToUser();
			});

			startTimerHz(4);
		}

		void timerCallback() override
		{
			if (editor.isShowing())
				editor.enable();
		}

		void resized() override
		{
			layout.resized();

			layout.place(editor, 0, 0, 3, 1, false);
			layout.place(date, 0, 1, 3, 1, false);
			layout.place(manifest, 0, 2, 1, 1, false);
			layout.place(inspire, 1, 2, 1, 1, false);
			layout.place(revealFolder, 2, 2, 1, 1, false);
		}

		void paint(Graphics&) override
		{}

		TextEditor editor;
		Label date;
		Button manifest, inspire, revealFolder;

	private:
		String getFolder()
		{
			auto specialLoc = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);

			return specialLoc.getFullPathName() + "\\Mrugalla\\sharedState\\TheManifestOfWisdom\\";
		}

		void saveToDisk()
		{
			if (editor.isEmpty())
				return parse("You have to enter some wisdom in order to manifest it.");

			const auto now = Time::getCurrentTime();
			const auto nowStr = now.toString(true, true, false, true).replaceCharacters(" ", "_").replaceCharacters(":", "_");

			File file(getFolder() + nowStr + ".txt");

			if (!file.existsAsFile())
				file.create();
			else
				return parse("Relax! You can only manifest 1 wisdom per minute.");

			file.appendText(editor.getText());

			parse("Manifested: " + nowStr);
		}

		void parse(String&& msg)
		{
			date.setText(msg);
			date.repaint();
		}
	};

	/// MENU STUFF IN GENERAL:

	struct ComponentWithBounds
	{
		/*comp,bounds,isQuad*/
		template<typename CompType>
		ComponentWithBounds(CompType* = nullptr, BoundsF&& = BoundsF(0.f, 0.f, 1.f, 1.f), bool = false);

		std::unique_ptr<Component> c;
		BoundsF b;
		bool isQuad;
	};

	struct CompModular :
		public Comp
	{
		/*utils,tooltip,cursorType*/
		CompModular(Utils&, String&&, CursorType);

		void init();

		std::vector<ComponentWithBounds> comps;
	protected:
		void paint(Graphics&) override;

		void resized() override;
	};

	class NavBar :
		public Comp
	{
		struct Node
		{
			Node(const ValueTree&, int, int);

			const ValueTree vt;
			const int x, y;
		};

		using Nodes = std::vector<Node>;

		Nodes makeNodes(const ValueTree&, int = 0, int = 0);

		int getDeepestNode() const noexcept;

	public:
		NavBar(Utils&, const ValueTree&);

		/*subMenu,parent*/
		void init(std::unique_ptr<CompModular>&, Comp&);

	protected:
		Label label;
		Nodes nodes;
		std::vector<std::unique_ptr<Button>> buttons;
		const int numMenus, deepestNode;

		void paint(Graphics&) override;

		void resized() override;
	};

	struct Menu :
		public CompWidgetable
	{
		Menu(Utils&, const ValueTree&);

	protected:
		Label label;
		NavBar navBar;
		std::unique_ptr<CompModular> subMenu;

		void paint(juce::Graphics&) override;

		void resized() override;
	};
}