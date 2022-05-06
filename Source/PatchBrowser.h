#pragma once
#include "Button.h"
#include "State.h"

#include <vector>
#include <memory>

namespace gui
{
	using ValueTree = juce::ValueTree;
	using Identifier = juce::Identifier;

	using Tags = std::vector<Identifier>;

	struct Patch :
		public Comp
	{
		using Patches = std::vector<std::unique_ptr<Patch>>;

		Patch(Utils& u, const String& _name, const String& _author) :
			Comp(u),
			name(u, _name),
			author(u, "by " + _author),
			remove(u, "Click here to remove this patch."),
			tags(),
			state(utils.getState())
		{
			layout.init(
				{ 8, 5, 1 },
				{ 1 }
			);

			name.textCID = ColourID::Txt;
			author.textCID = ColourID::Hover;

			name.mode = Label::Mode::TextToLabelBounds;
			author.mode = Label::Mode::TextToLabelBounds;

			makeTextButton(remove, "X", false);
			remove.getLabel().textCID = ColourID::Abort;
			remove.onClick.push_back([&]()
			{

			});

			addAndMakeVisible(name);
			addAndMakeVisible(author);
			addAndMakeVisible(remove);
		}

		bool operator==(const String & other) const noexcept
		{
			return name.getText() == other;
		}

		bool operator==(const Patch& other) const noexcept
		{
			return name.getText() == other.name.getText();
		}

		bool has(const Identifier& _tag) const noexcept
		{
			for (const auto& tag : tags)
				if (tag == _tag)
					return true;
			return false;
		}

		void paint(Graphics&) override
		{

		}

		void resized() override
		{
			layout.resized();

			layout.place(name, 0, 0, 1, 1, false);
			layout.place(author, 1, 0, 1, 1, false);
			layout.place(remove, 2, 0, 1, 1, true);
		}

		Label name, author;
		Button remove;
		Tags tags;
		ValueTree state;
	};

	String toString(const Patch& patch)
	{
		String str("name: " + patch.name.getText() +
			"\nauthor: " + patch.author.getText() +
			"\ntags: ");
		for (const auto& tag : patch.tags)
			str += String(tag.toString()) + "; ";

		return str;
	}

	using Patches = Patch::Patches;

	struct PatchList :
		public Comp
	{
		static constexpr float RelHeight = 10.f;

		PatchList(Utils& u, Tags& _tags) :
			Comp(u, "", CursorType::Default),
			patches(),
			filterString(""),
			tags(_tags),
			selected(-1)
		{}

		bool save(const String& _name, const String& _author)
		{
			for (const auto& patch : patches)
			{
				if (*patch == _name)
					return false;
			}

			patches.push_back(std::make_unique<Patch>(
				utils,
				patches,
				_name,
				_author
			));

			addAndMakeVisible(*patches.back());
			updateShown();

			return true;
		}

		const Patch* getSelectedPatch() const noexcept
		{
			if (selected == -1 || patches.empty())
				return nullptr;
			return patches[selected].get();
		}

		void show(const String& containedString)
		{
			if (filterString == containedString)
				return;

			filterString = containedString;

			updateShown();
		}

		void show(const Identifier& _tag, bool isAdded)
		{
			if (isAdded)
				tags.push_back(_tag);
			else
				for(auto t = 0; t < tags.size(); ++t)
					if (tags[t] == _tag)
					{
						tags.erase(tags.begin() + t);
						break;
					}

			updateShown();
		}

	protected:
		Patches patches;
		String filterString;
		Tags& tags;
		int selected;

		void resized() override
		{
			if (patches.empty())
				return;

			auto x = 0.f;
			auto y = 0.f;
			auto w = static_cast<float>(getWidth());
			auto h = utils.thicc() * RelHeight;

			// show patches that are visible
			// in respect to scrollbar position

			for (auto p = 0; p < patches.size(); ++p)
			{
				auto& patch = patches[p];

				patch->setBounds(BoundsF(x, y, w, h).toNearestInt());
				y += h;
			}
		}

		void paint(Graphics& g) override
		{
			if (patches.empty())
			{
				g.setColour(Colours::c(ColourID::Abort));
				g.setFont(getFontLobster().withHeight(24.f));
				g.drawFittedText(
					"sry, this browser does not contain patches yet...",
					getLocalBounds(),
					Just::centred,
					1
				);
				return;
			}

			{
				auto x = 0.f;
				auto w = static_cast<float>(getWidth());
				auto h = static_cast<float>(getHeight());
				auto r = utils.thicc() * RelHeight;
				bool paintRow = true;

				g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
				for (auto y = 0.f; y < h; y += r, paintRow = !paintRow)
					if (paintRow)
						g.fillRect(x, y, w, r);
			}
			
		}

		void updateShown()
		{
			bool considerTags = tags.size() != 0;
			bool considerString = filterString.isNotEmpty();

			if(!considerTags && !considerString)
				for (auto& patch : patches)
					patch->setVisible(true);
			else
			{
				for (auto& patch : patches)
					patch->setVisible(false);

				if(considerString)
					for (auto& patch : patches)
					{
						const auto& patchName = patch->name.getText();
						if (patchName.contains(filterString))
							patch->setVisible(true);
					}

				if(considerTags)
					for (auto& patch : patches)
					{
						for (const auto& tag : tags)
						{
							if (patch->has(tag))
								patch->setVisible(true);
						}
					}
			}

			resized();
			repaint();
		}
	};

	struct PatchInspector :
		public Comp
	{
		PatchInspector(Utils& u, PatchList& _patchList) :
			Comp(u, "", CursorType::Default),
			patchList(_patchList)
		{}

		void paint(Graphics& g) override
		{
			auto patch = patchList.getSelectedPatch();
			const auto thicc = utils.thicc();
			g.setColour(Colours::c(ColourID::Hover));
			g.drawRoundedRectangle(getLocalBounds().toFloat(), thicc, thicc);
			g.setFont(getFontLobster().withHeight(24.f));

			if (patch == nullptr)
			{
				g.drawFittedText("inspector:\nno patch selected.", getLocalBounds(), Just::centred, 1);
			}
			else
			{
				g.drawFittedText(
					toString(*patch),
					getLocalBounds(),
					Just::centred,
					1
				);
			}
		}

	protected:
		PatchList& patchList;
	};

	struct PatchBrowser :
		public Comp,
		public Timer
	{
		PatchBrowser(Utils& u) :
			Comp(u, "", CursorType::Default),
			Timer(),
			tags(),
			closeButton(u, "Click here to close the browser."),
			saveButton(u, "Click here to save this patch."),
			searchBar(u, "Define a name or search for a patch.", "Init.."),
			patchList(u, tags),
			inspector(u, patchList)
		{
			layout.init(
				{ 1, 2, 2, 8, 21, 2, 2, 1 },
				{ 1, 2, 2, 8, 21, 1 }
			);

			makeTextButton(closeButton, "X", false);
			closeButton.getLabel().mode = Label::Mode::TextToLabelBounds;
			closeButton.getLabel().textCID = ColourID::Abort;
			closeButton.onClick.push_back([&]()
			{
				setVisible(false);
			});

			makeTextButton(saveButton, "save", false);
			saveButton.getLabel().mode = Label::Mode::TextToLabelBounds;
			saveButton.onClick.push_back([&]()
			{
				if (searchBar.isNotEmpty())
				{
					if (patchList.save(searchBar.getText(), "max musterboy"))
					{
						searchBar.clear();
					}
				}
			});

			addAndMakeVisible(closeButton);
			addAndMakeVisible(saveButton);
			addAndMakeVisible(searchBar);
			addAndMakeVisible(patchList);
			addAndMakeVisible(inspector);

			setOpaque(true);
		}

		void setVisible(bool e) override
		{
			if (e)
			{
				Comp::setVisible(e);
				searchBar.enable();
				startTimerHz(12);
			}
			else
			{
				stopTimer();
				searchBar.disable();
				Comp::setVisible(e);
			}
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xff000000));

			g.setColour(Colour(0x44ffffff));
			//layout.paint(g);

			layout.label(g, "v", 2, 1, 1, 1, true);
			layout.label(g, ">", 6, 1, 1, 1, true);
			layout.label(g, "bar with sortables", 4, 2, 2, 1, false);
			layout.label(g, "tags/filters/categories etc", 1, 2, 3, 2, false);
			layout.label(g, "s\nc\nr\no\nl\nl", 6, 2, 1, 3, false);
		}

		void resized() override
		{
			layout.resized();

			layout.place(closeButton, 1, 1, 1, 1, true);
			layout.place(saveButton, 5, 1, 1, 1, true);

			layout.place(searchBar, 3, 1, 2, 1, false);
			layout.place(patchList, 4, 3, 2, 2, false);
			layout.place(inspector, 1, 4, 3, 1, false);
		}

		void timerCallback() override
		{
			// update patch(es') infos if needed..
			
			patchList.show(searchBar.getText());

		}

		const String getSelectedPatchName() const noexcept
		{
			const auto patch = patchList.getSelectedPatch();
			if(patch != nullptr)
				return patch->name.getText();
			return "init patch";
		}

	protected:
		Tags tags;
		Button closeButton;
		Button saveButton;
		TextEditor searchBar;
		PatchList patchList;
		PatchInspector inspector;
	};

	struct ButtonPatchBrowser :
		public Button
	{
		ButtonPatchBrowser(Utils& u, PatchBrowser& _browser) :
			Button(u, "Click here to open the patch browser."),
			browser(_browser)
		{
			makeTextButton(*this, browser.getSelectedPatchName(), false);
			onClick.push_back([&]()
			{
				const auto e = browser.isVisible();
				if(e)
					browser.setVisible(false);
				else
				{
					browser.setVisible(true);
				}
			});
		}

	protected:
		PatchBrowser& browser;
	};
}