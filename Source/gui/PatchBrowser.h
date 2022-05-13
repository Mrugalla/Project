#pragma once
#include "Button.h"

namespace gui
{
	using ValueTree = juce::ValueTree;
	using Identifier = juce::Identifier;

	using Tags = std::vector<Identifier>;

	struct Patch :
		public Button
	{
		using SharedPatch = std::shared_ptr<Patch>;
		using WeakPatch = std::weak_ptr<Patch>;
		using Patches = std::vector<SharedPatch>;

		Patch(Utils& u, const String& _name, const String& _author) :
			Button(u, "Click here to select and load this patch."),
			name(u, _name),
			author(u, "by " + _author),
			tags(),
			state(utils.getState())
		{
			layout.init(
				{ 1, 8, 8, 1 },
				{ 1 }
			);

			makeSymbolButton(*this, ButtonSymbol::Empty);

			name.font = getFontDosisMedium();
			author.font = name.font;

			name.textCID = ColourID::Txt;
			author.textCID = ColourID::Hover;

			name.just = Just::centredLeft;
			author.just = Just::centredLeft;

			name.mode = Label::Mode::TextToLabelBounds;
			author.mode = Label::Mode::TextToLabelBounds;

			addAndMakeVisible(name);
			addAndMakeVisible(author);
		}

		bool isSame(const String& _name, const String& _author) const
		{
			return name.getText() == _name &&
				author.getText() == _author;
		}

		bool isSame(Patch& other) const
		{
			return isSame(other.name.getText(), other.author.getText());
		}

		bool isRemovable() const
		{
			return !author.getText().contains("factory");
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

			layout.place(name, 1, 0, 1, 1, false);
			layout.place(author, 2, 0, 1, 1, false);
		}

		Label name, author;
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

	using SharedPatch = Patch::SharedPatch;
	using WeakPatch = Patch::WeakPatch;
	using Patches = Patch::Patches;

	struct PatchList :
		public CompScrollable
	{
		static constexpr float RelHeight = 10.f;

		PatchList(Utils& u, Tags& _tags) :
			CompScrollable(u),
			patches(),
			filterString(""),
			tags(_tags),
			selected(nullptr),
			listBounds()
		{
			layout.init(
				{ 21, 1 },
				{ 1 }
			);

			Random rand;
			for (auto i = 0; i < 1000; ++i)
			{
				String strA, strB;
				appendRandomString(strA, rand, 12);
				appendRandomString(strB, rand, 12);
				save(strA, strB);
			}
		}

		bool save(const String& _name, const String& _author)
		{
			// check if patch already exists (accept overwrite?)
			for (const auto& patch : patches)
				if (patch->isSame(_name, _author))
					return false;

			patches.push_back(std::make_shared<Patch>(
				utils,
				_name,
				_author
			));

			selected = patches.back();

			selected->onClick.push_back([&, thisPatch = std::weak_ptr<Patch>(selected)]()
			{
				auto locked = thisPatch.lock();

				if (locked == nullptr)
					return;

				selected = locked;
				updateShown();
			});

			addAndMakeVisible(*selected);
			updateShown();

			return true;
		}

		void removeSelected()
		{
			if (selected == nullptr)
				return;

			for (auto i = 0; i < patches.size(); ++i)
			{
				auto ptch = patches[i];
				if (ptch == selected)
				{
					if (ptch->isRemovable())
					{
						removeChildComponent(ptch.get());
						patches.erase(patches.begin() + i);
						selected.reset();
						updateShown();
						return;
					}
				}
			}
		}

		SharedPatch getSelectedPatch() const noexcept
		{
			if (selected == nullptr || patches.empty())
				return nullptr;
			return selected;
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
		SharedPatch selected;
		BoundsF listBounds;
		float maxHeight;

		void resized() override
		{
			layout.resized();

			layout.place(scrollBar, 1, 0, 1, 1);

			listBounds = layout(0, 0, 1, 1);

			const auto x = listBounds.getX();
			const auto w = listBounds.getWidth();
			const auto h = utils.thicc() * RelHeight;
			maxHeight = h * static_cast<float>(patches.size()) - listBounds.getHeight();

			auto y = listBounds.getY() - yScrollOffset * maxHeight;

			for (auto p = 0; p < patches.size(); ++p)
			{
				auto& patch = patches[p];

				if (patch->isVisible())
				{
					patch->setBounds(BoundsF(x, y, w, h).toNearestInt());
					y += h;
				}
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

			paintList(g);
		}

		void paintList(Graphics& g)
		{
			auto x = listBounds.getX();
			auto y = listBounds.getY() - yScrollOffset * maxHeight;
			auto w = listBounds.getWidth();
			auto btm = listBounds.getBottom();
			auto r = utils.thicc() * RelHeight;

			g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
			for (auto i = 0; i < patches.size(); ++i)
			{
				if (y >= btm)
					return;
				if (i % 2 == 0)
					g.fillRect(x, y, w, r);

				const auto ptch = patches[i];

				if (ptch->isVisible() && y >= 0.f)
				{
					if (selected == ptch)
					{
						g.setColour(Colours::c(ColourID::Interact));
						g.drawRect(x, y, w, r);
						g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
					}
				}
				y += r;
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
				{
					patch->setVisible(false);
				}

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
			repaintWithChildren(getParentComponent());
		}
	};

	struct PatchList2 :
		public Comp
	{
		static constexpr float RelHeight = 10.f;

		PatchList2(Utils& u, Tags& _tags) :
			Comp(u, "", CursorType::Default),
			sortByName(u, "click here to sort patches by name."),
			sortByAuthor(u, "click here to sort patches by author."),
			patches(),
			filterString(""),
			tags(_tags),
			selected(nullptr)
		{
			layout.init(
				{ 1, 8, 8, 1 },
				{ 1 }
			);

			addAndMakeVisible(sortByName);
			addAndMakeVisible(sortByAuthor);

			makeToggleButton(sortByName, "name");
			makeToggleButton(sortByAuthor, "author");

			sortByName.onClick.push_back([&]()
				{
					auto sortFunc = [&ts = sortByName.toggleState](const SharedPatch& a, const SharedPatch& b)
					{
						if (ts)
							return a->name.getText().compareNatural(b->name.getText()) > 0;
						else
							return a->name.getText().compareNatural(b->name.getText()) < 0;
					};

					std::sort(patches.begin(), patches.end(), sortFunc);

					updateShown();
				});

			sortByAuthor.onClick.push_back([&]()
				{
					auto sortFunc = [&ts = sortByName.toggleState](const SharedPatch& a, const SharedPatch& b)
					{
						if (ts)
							return a->author.getText().compareNatural(b->author.getText()) > 0;
						else
							return a->author.getText().compareNatural(b->author.getText()) < 0;
					};

					std::sort(patches.begin(), patches.end(), sortFunc);

					updateShown();
				});

			{
				auto& nLabel = sortByName.getLabel();
				auto& authLabel = sortByAuthor.getLabel();

				nLabel.textCID = ColourID::Hover;
				authLabel.textCID = nLabel.textCID;

				nLabel.font = getFontDosisMedium();
				authLabel.font = nLabel.font;

				nLabel.just = Just::centredLeft;
				authLabel.just = nLabel.just;

				nLabel.mode = Label::Mode::TextToLabelBounds;
				authLabel.mode = nLabel.mode;
			}

			Random rand;
			for (auto i = 0; i < 200; ++i)
			{
				String strA, strB;
				appendRandomString(strA, rand, 12);
				appendRandomString(strB, rand, 12);
				save(strA, strB);
			}
		}

		bool save(const String& _name, const String& _author)
		{
			// check if patch already exists (accept overwrite?)
			for (const auto& patch : patches)
				if (patch->isSame(_name, _author))
					return false;

			patches.push_back(std::make_shared<Patch>(
				utils,
				_name,
				_author
				));

			selected = patches.back();

			selected->onClick.push_back([&, thisPatch = std::weak_ptr<Patch>(selected)]()
			{
				auto locked = thisPatch.lock();

				if (locked == nullptr)
					return;

				selected = locked;
				updateShown();
			});

			addAndMakeVisible(*selected);
			updateShown();

			return true;
		}

		void removeSelected()
		{
			if (selected == nullptr)
				return;

			for (auto i = 0; i < patches.size(); ++i)
			{
				auto ptch = patches[i];
				if (ptch == selected)
				{
					if (ptch->isRemovable())
					{
						removeChildComponent(ptch.get());
						patches.erase(patches.begin() + i);
						selected.reset();
						updateShown();
						return;
					}
				}
			}
		}

		SharedPatch getSelectedPatch() const noexcept
		{
			if (selected == nullptr || patches.empty())
				return nullptr;
			return selected;
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
				for (auto t = 0; t < tags.size(); ++t)
					if (tags[t] == _tag)
					{
						tags.erase(tags.begin() + t);
						break;
					}

			updateShown();
		}

	protected:
		Button sortByName, sortByAuthor;
		Patches patches;
		String filterString;
		Tags& tags;
		SharedPatch selected;

		void resized() override
		{
			layout.resized();

			if (patches.empty())
				return;

			auto y = 0.f;
			auto h = utils.thicc() * RelHeight;

			{
				auto w = layout.getW(1);
				auto x = layout.getX(1);
				sortByName.setBounds(BoundsF(x, y, w, h).toNearestInt());

				x = layout.getX(2);
				sortByAuthor.setBounds(BoundsF(x, y, w, h).toNearestInt());
			}

			y += h;
			const auto x = 0.f;
			auto w = static_cast<float>(getWidth());

			// show patches that are visible
			// in respect to scrollbar position

			for (auto p = 0; p < patches.size(); ++p)
			{
				auto& patch = patches[p];

				if (patch->isVisible())
				{
					patch->setBounds(BoundsF(x, y, w, h).toNearestInt());
					y += h;
				}
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

			paintList(g);
		}

		void paintList(Graphics& g)
		{
			auto x = 0.f;
			auto y = 0.f;
			auto w = static_cast<float>(getWidth());
			auto h = static_cast<float>(getHeight());
			auto r = utils.thicc() * RelHeight;

			y += r;

			g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
			for (auto i = 0; i < patches.size(); ++i)
			{
				if (i % 2 == 0)
					g.fillRect(x, y, w, r);

				if (y >= 0.f)
				{
					const auto ptch = patches[i];

					if (ptch->isVisible() && ptch->getY() < getHeight())
					{
						if (selected == ptch)
						{
							g.setColour(Colours::c(ColourID::Interact));
							g.drawRect(x, y, w, r);
							g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
						}

						y += r;
						if (y > h)
							return;
					}
				}
			}
		}

		void updateShown()
		{
			bool considerTags = tags.size() != 0;
			bool considerString = filterString.isNotEmpty();

			if (!considerTags && !considerString)
				for (auto& patch : patches)
					patch->setVisible(true);
			else
			{
				for (auto& patch : patches)
				{
					patch->setVisible(false);
				}

				if (considerString)
					for (auto& patch : patches)
					{
						const auto& patchName = patch->name.getText();
						if (patchName.contains(filterString))
							patch->setVisible(true);
					}

				if (considerTags)
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
			repaintWithChildren(getParentComponent());
		}
	};

	struct PatchInspector :
		public Comp
	{
		PatchInspector(Utils& u, PatchList& _patchList) :
			Comp(u, "", CursorType::Default),
			patchList(_patchList),
			title(u, "Inspector:"),
			name(u, "Name: "),
			author(u, "Author: "),
			tags(u, "Tags: "),
			tagsEditor(u, "Seperate each tag with a semicolon. ';'", "no tags yet"),
			patch(nullptr)
		{
			layout.init(
				{ 1, 5, 1 },
				{ 1, 5, 5, 5, 5, 5, 1 }
			);

			addAndMakeVisible(title);
			title.mode = Label::Mode::TextToLabelBounds;
			title.textCID = ColourID::Hover;
			title.font = getFontLobster();

			addAndMakeVisible(name);
			addAndMakeVisible(author);
			addAndMakeVisible(tags);
			addAndMakeVisible(tagsEditor);

			name.mode = Label::Mode::TextToLabelBounds;
			name.just = Just::centredLeft;
			title.textCID = ColourID::Txt;
			title.font = getFontDosisBold();

			author.mode = Label::Mode::TextToLabelBounds;
			author.textCID = title.textCID;
			author.font = title.font;
			author.just = title.just;
			tags.mode = Label::Mode::TextToLabelBounds;
			tags.textCID = title.textCID;
			tags.font = title.font;
			tags.just = title.just;
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc();
			g.setColour(Colours::c(ColourID::Hover));
			g.drawRoundedRectangle(getLocalBounds().toFloat(), thicc, thicc);
			g.setFont(getFontDosisMedium().withHeight(24.f));

			if (patch == nullptr)
			{
				g.drawFittedText("inspector:\n\nno patch selected.", getLocalBounds(), Just::centred, 1);
			}
			else
			{
				//g.setColour(Colours::c(ColourID::Txt));
				//g.drawFittedText(
				//	"inspector:\n\n" + 
				//	toString(*patch),
				//	getLocalBounds(),
				//	Just::centred,
				//	1
				//);
			}
		}

		void update()
		{
			auto sel = patchList.getSelectedPatch();
			if (patch == sel)
				return;

			patch = sel;
			repaintWithChildren(this);
		}

	protected:
		PatchList& patchList;
		Label title, name, author, tags;
		TextEditor tagsEditor;
		SharedPatch patch;

		void resized() override
		{
			layout.resized();

			layout.place(tagsEditor, 1, 5, 1, 1, false);

			std::array<Label*, 4> labelPtr{ &title, &name, &author, &tags };

			for (auto l : labelPtr)
				l->mode = Label::Mode::TextToLabelBounds;

			layout.place(title, 1, 1, 1, 1, false);
			layout.place(name, 1, 2, 1, 1, false);
			layout.place(author, 1, 3, 1, 1, false);
			layout.place(tags, 1, 4, 1, 1, false);

			auto minHeight = labelPtr.front()->font.getHeight();
			for (auto i = 1; i < labelPtr.size(); ++i)
			{
				const auto& l = *labelPtr[i];
				const auto nHeight = l.font.getHeight();
				if (minHeight < nHeight)
					minHeight = nHeight;
			}

			for (auto l : labelPtr)
			{
				l->mode = Label::Mode::None;
				l->setMinFontHeight(minHeight);
			}
		}
	};

	struct PatchBrowser :
		public CompScreenshotable,
		public Timer
	{
		PatchBrowser(Utils& u) :
			CompScreenshotable(u),
			Timer(),
			tags(),
			closeButton(u, "Click here to close the browser."),
			saveButton(u, "Click here to save this patch."),
			removeButton(u, "Click here to remove this patch."),
			searchBar(u, "Define a name or search for a patch.", "Init.."),
			patchList(u, tags)
		{
			layout.init(
				{ 1, 2, 34, 2, 2, 1 },
				{ 1, 2, 34, 1 }
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
					if (patchList.save(searchBar.getText(), "user"))
					{
						searchBar.clear();
						repaintWithChildren(this);
					}
				}
			});

			searchBar.onReturn = saveButton.onClick.back();

			makeTextButton(removeButton, "rmv", false);
			removeButton.getLabel().textCID = ColourID::Abort;
			removeButton.onClick.push_back([&]()
			{
				patchList.removeSelected();
			});

			addAndMakeVisible(closeButton);
			addAndMakeVisible(saveButton);
			addAndMakeVisible(removeButton);
			addAndMakeVisible(searchBar);
			addAndMakeVisible(patchList);

			onScreenshotFX.push_back([](Graphics& g, Image& img)
				{
					imgPP::blur(img, g, 7);

					for (auto y = 0; y < img.getHeight(); ++y)
						for (auto x = 0; x < img.getWidth(); ++x)
							img.setPixelAt
							(x, y,
								img.getPixelAt(x, y)
								.withMultipliedSaturation(.4f)
								.withMultipliedBrightness(.4f)
							);
				});
		}

		void setVisible(bool e) override
		{
			if (e)
			{
				notify(EvtType::BrowserOpened);
				takeScreenshot();
				Comp::setVisible(e);
				searchBar.enable();
				startTimerHz(12);
			}
			else
			{
				notify(EvtType::BrowserClosed);
				stopTimer();
				searchBar.disable();
				Comp::setVisible(e);
			}
		}

		void paint(Graphics& g) override
		{
			CompScreenshotable::paint(g);
			//g.fillAll(Colour(0xff000000));

			//g.setColour(Colour(0x44ffffff));
			//layout.paint(g);

			//layout.label(g, "s\nc\nr\no\nl\nl", 4, 2, 1, 3, false);
		}

		void resized() override
		{
			CompScreenshotable::resized();

			layout.resized();

			layout.place(closeButton, 1, 1, 1, 1, true);
			layout.place(saveButton, 3, 1, 1, 1, true);
			layout.place(removeButton, 4, 1, 1, 1, true);

			layout.place(searchBar, 2, 1, 2, 1, false);
			layout.place(patchList, 1, 2, 4, 2, false);
		}

		void timerCallback() override
		{
			patchList.show(searchBar.getText());
		}

		String getSelectedPatchName() const
		{
			const auto patch = patchList.getSelectedPatch();
			if(patch != nullptr)
				return patch->name.getText();
			return "init";
		}

	protected:
		Tags tags;
		Button closeButton;
		Button saveButton, removeButton;
		TextEditor searchBar;
		PatchList patchList;
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