#pragma once
#include "TextEditor.h"

#define DebugNumPatches 30

// OLD STUFF:

// VERSION 1
/*
	struct PatchInspector :
		public Comp
	{
		struct Tag :
			public Button
		{
			Tag(Utils& u, const String& str) :
				Button(u, "Click on this tag to select it.")
			{
				makeToggleButton(*this, str);
			}
		};

		struct Tags :
			public CompScrollable
		{
			static constexpr float RelWidth = 20.f;

			Tags(Utils& u) :
				CompScrollable(u, false),
				tags()
			{
				setInterceptsMouseClicks(false, true);

				layout.init(
					{ 1 },
					{ 5, 3 }
				);
			}

			void updatePatch(const SharedPatch& patch)
			{
				clear();
				for (const auto& tag : patch->tags)
					addTag(tag);
			}

			bool addTag(const String& str, SharedPatch& patch)
			{
				if(addTag(str))
					return patch->addTag(str);
				return false;
			}

			void clear()
			{
				tags.clear();
			}

			void removeSelected(SharedPatch& patch, TagsSelector& tagsSelector)
			{
				auto numTags = static_cast<int>(tags.size());
				for (auto i = 0; i < numTags; ++i)
				{
					const auto& tag = *tags[i];
					if (tag.toggleState == 1)
					{
						patch->removeTag(tag.getText());
						tagsSelector.removeTag(tag.getText());

						tags.erase(tags.begin() + i);
						resized();

						if (numTags != 1)
							groupTags();

						return;
					}
				}

			}

			std::vector<std::unique_ptr<Tag>> tags;

			void resized() override
			{
				for (auto& t : tags)
					t->getLabel().mode = Label::Mode::TextToLabelBounds;

				const auto numTags = tags.size();
				if (numTags == 0)
					return;

				layout.resized();
				layout.place(scrollBar, 0, 1, 1, 1, false);

				const auto tagsArea = layout(0, 0, 1, 1, false);
				{
					const auto thicc = utils.thicc;
					const auto y = tagsArea.getY();
					const auto h = tagsArea.getHeight();
					const auto w = thicc * RelWidth;

					actualHeight = w * static_cast<float>(numTags);

					auto x = tagsArea.getX() - xScrollOffset;
					for (auto i = 0; i < numTags; ++i)
					{
						auto& tag = *tags[i];
						tag.setBounds(BoundsF(x, y, w, h).toNearestInt());
						x += w;
					}
				}
			}

		private:
			void paint(Graphics& g) override
			{
				g.fillAll(Colours::c(ColourID::Hover));
			}

			void groupTags()
			{
				for (auto& tag : tags)
					tag->toggleState = 0;
				tags.back()->toggleState = 1;

				for (auto i = 0; i < tags.size(); ++i)
				{
					auto& tag = *tags[i];

					tag.onClick.clear();
					tag.onClick.push_back([&others = tags, i]()
						{
							for (auto& oth : others)
							{
								oth->toggleState = 0;
								repaintWithChildren(oth.get());
							}

							others[i]->toggleState = 1;
						});
				}


				repaintWithChildren(getParentComponent());
			}

			bool addTag(const String& str)
			{
				for (auto& t : tags)
					if (t->getText() == str)
						return false;

				tags.push_back(std::make_unique<Tag>(utils, str));
				addAndMakeVisible(*tags.back());
				resized();

				groupTags();

				return true;
			}
		};

		PatchInspector(Utils& u, PatchList& _patchList, TagsSelector& _tagsSelector) :
			Comp(u, "", CursorType::Default),
			patchList(_patchList),
			name(u, "Name: "),
			author(u, "Author: "),
			patch(nullptr),

			tags(u),
			addTag(u, "Click here to add a new tag!"),
			removeTag(u, "Click here to remove the selected tag!"),
			tagEditor(u, "Type a tag name!", "Enter tag.."),

			tagsSelector(_tagsSelector)
		{
			layout.init(
				{ 1, 13, 21, 2 },
				{ 1, 13, 13, 1 }
			);

			{
				addAndMakeVisible(name);
				addAndMakeVisible(author);

				name.mode = Label::Mode::TextToLabelBounds;
				name.just = Just::centredLeft;
				name.textCID = ColourID::Txt;
				name.font = getFontDosisBold();

				author.mode = name.mode;
				author.textCID = name.textCID;
				author.font = name.font;
				author.just = name.just;
			}

			{
				addAndMakeVisible(tags);
				addAndMakeVisible(addTag);
				addAndMakeVisible(removeTag);
				addChildComponent(tagEditor);

				makeTextButton(addTag, "+");
				makeTextButton(removeTag, "-");

				addTag.onClick.push_back([&]()
				{
					if (!tagEditor.isEnabled())
					{
						tagEditor.enable();
						resized();
					}
					else
					{
						addTagFromEditor();
					}
				});

				removeTag.onClick.push_back([&]()
				{
					tags.removeSelected(patch, tagsSelector);
				});

				tagEditor.onReturn = [&]()
				{
					addTagFromEditor();
				};

				tagEditor.onEscape = [&]()
				{
					tagEditor.setVisible(false);
				};
			}
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc;
			g.setColour(Colours::c(ColourID::Bg));
			g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(thicc), thicc);
		}

		void update()
		{
			auto sel = patchList.getSelectedPatch();
			if (patch == sel)
				return;

			patch = sel;
			if (patch != nullptr)
			{
				name.setText("Name: " + patch->name.getText());
				author.setText("Author: " + patch->author.getText());
				tags.updatePatch(patch);
			}

			repaintWithChildren(this);
		}

	protected:
		PatchList& patchList;
		Label name, author;
		SharedPatch patch;

		Tags tags;
		Button addTag, removeTag;
		TextEditor tagEditor;

		TagsSelector& tagsSelector;

		void resized() override
		{
			layout.resized();

			std::array<Label*, 2> labelPtr { &name, &author };

			for (auto l : labelPtr)
				l->mode = Label::Mode::TextToLabelBounds;

			layout.place(name, 1, 1, 2, 1, false);
			layout.place(author, 1, 2, 2, 1, false);

			{
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

			layout.place(tags, 2, 1, 1, 2, false);
			layout.place(addTag, 3, 1, 1, 1, true);
			layout.place(removeTag, 3, 2, 1, 1, true);
			layout.place(tagEditor, 2.2f, 1.2f, .6f, 1.6f, false);
		}

		void mouseUp(const Mouse& evt)
		{
			Comp::mouseUp(evt);
			tagEditor.setVisible(false);
		}

		void addTagFromEditor()
		{
			if (tags.addTag(tagEditor.getText(), patch))
				tagsSelector.addTag(tagEditor.getText());
			tagEditor.setVisible(false);
		}
	};

}

*/

// VERSION 2
	/*
	struct Patches
	{
		Patches() :
			patches(),
			selected(nullptr)
		{}

		int getIndex(const Patch& nPatch) const noexcept
		{
			for (auto i = 0; i < patches.size(); ++i)
				if (nPatch == patches[i])
					return i;
			return -1;
		}

		bool contains(const Patch& nPatch) const noexcept
		{
			return getIndex(nPatch) != -1;
		}

		bool save(const String& name, const String& author)
		{
			if (name.isEmpty() || author.isEmpty())
				return false;

			Patch nPatch(name, author);

			if (contains(nPatch))
				return false;

			patches.push_back(nPatch);
			select(&patches.back());

			return true;
		}

		bool removeSelected()
		{
			if (selected == nullptr)
				return false;

			for(auto i = 0; i < patches.size(); ++i)
				if (patches[i] == *selected)
				{
					patches.erase(patches.begin() + i);
					select(nullptr);
					return true;
				}

			return false;
		}

		bool select(Patch* p) noexcept
		{
			if(p != nullptr)
				for(auto& ptchs: patches)
					if (ptchs == *p)
					{
						selected = p;
						return true;
					}

			selected = false;
			return false;
		}

		const Patch* getSelected() const noexcept
		{
			return selected;
		}

		Patch& back() noexcept { return patches.back(); }

		const Patch& back() const noexcept { return patches.back(); }

		Patch& operator[](int i) noexcept { return patches[i]; }

		const Patch& operator[](int i) const noexcept { return patches[i]; }

		size_t numPatches() const noexcept { return patches.size(); }
	protected:
		std::vector<Patch> patches;
		Patch* selected;
	};

	inline String toString(const Patches& p)
	{
		String str("Patches:\n");

		for (auto i = 0; i < p.numPatches(); ++i)
			str += toString(p[i]) + "\n";

		const auto sel = p.getSelected();
		if (sel != nullptr)
			str += "selected: " + sel->name;

		return str;
	}
	*/
	/*
		void filterEntries(const String& filterText)
		{
			const bool considerText = filterText.isNotEmpty();

			if (considerText)
			{
				for (auto i = 0; i < patchViews.size(); ++i)
				{
					auto& view = *patchViews[i];
					const auto& patch = patches[i];

					const bool validName = patch.name.contains(filterText);

					view.setVisible(validName);
				}
			}
			else
			{
				// no filters applied
				for (auto& view : patchViews)
					view->setVisible(true);
			}
		}

		
	};
	*/
	/*
	struct PatchesViewSortable :
		public Comp
	{
		using UniqueView = PatchesView::UniqueView;
		using SortFunc = PatchesView::SortFunc;

		PatchesViewSortable(Utils& u, Patches& patches) :
			Comp(u, "", CursorType::Default),
			view(u, patches),
			sortByName(u, "Click here to sort all patches by name."),
			sortByAuthor(u, "Click here to sort all patches by author.")
		{
			layout.init(
				{ 21, 1 },
				{ 2, 34 }
			);

			addAndMakeVisible(sortByName);
			addAndMakeVisible(sortByAuthor);

			makeTextButton(sortByName, "NAME");
			makeTextButton(sortByAuthor, "AUTHOR");

			sortByName.onClick.push_back([&]()
				{
					sortByName.toggleState = sortByName.toggleState == 0 ? 1 : 0;

					SortFunc sortFunc = [&ts = sortByName.toggleState](const UniqueView& a, const UniqueView& b)
					{
						const auto& pA = a->name.getText();
						const auto& pB = b->name.getText();

						if (ts == 1)
							return pA.compareNatural(pB) > 0;
						else
							return pA.compareNatural(pB) < 0;
					};

					view.sort(sortFunc);
				});

			sortByAuthor.onClick.push_back([&]()
				{
					sortByAuthor.toggleState = sortByAuthor.toggleState == 0 ? 1 : 0;

					SortFunc sortFunc = [&ts = sortByAuthor.toggleState](const UniqueView& a, const UniqueView& b)
					{
						const auto& pA = a->author.getText();
						const auto& pB = b->author.getText();

						if (ts == 1)
							return pA.compareNatural(pB) > 0;
						else
							return pA.compareNatural(pB) < 0;
					};

					view.sort(sortFunc);
				});

			{
				auto& nLabel = sortByName.getLabel();
				auto& authLabel = sortByAuthor.getLabel();

				nLabel.textCID = ColourID::Txt;
				authLabel.textCID = nLabel.textCID;

				nLabel.font = getFontDosisMedium();
				authLabel.font = nLabel.font;

				nLabel.just = Just::centredLeft;
				authLabel.just = nLabel.just;

				nLabel.mode = Label::Mode::TextToLabelBounds;
				authLabel.mode = nLabel.mode;
			}

			addAndMakeVisible(view);
		}

		void select(const Patch* selected) noexcept
		{
			view.select(selected);
		}

		PatchView& addView()
		{
			return view.addView();
		}

		bool remove(const Patch* selected)
		{
			return view.remove(selected);
		}

		void filterEntries(const String& filterText)
		{
			view.filterEntries(filterText);
		}

		void resized() override
		{
			layout.resized();

			auto buttonArea = layout(0, 0, 1, 1);
			{
				auto x = buttonArea.getX();
				auto y = buttonArea.getY();
				auto w = buttonArea.getWidth();
				auto h = buttonArea.getHeight();

				const auto bothWidth = 1.f / static_cast<float>(PatchViewNameWidth + PatchViewAuthorWidth);
				const auto nameW = w * static_cast<float>(PatchViewNameWidth) * bothWidth;
				const auto authorW = w * static_cast<float>(PatchViewAuthorWidth) * bothWidth;

				sortByName.setBounds(BoundsF(x, y, nameW, h).toNearestInt());
				x += nameW;
				sortByAuthor.setBounds(BoundsF(x, y, authorW, h).toNearestInt());
			}

			layout.place(view, 0, 1, 2, 1, false);
		}

	protected:
		PatchesView view;
		Button sortByName, sortByAuthor;

		void paint(Graphics&) override {}
	};
	*/
	/*
	struct PatchBrowser :
		public CompScreenshotable
	{
		PatchBrowser(Utils& u) :
			CompScreenshotable(u),

			closeButton(u, "Click here to close the browser."),
			saveButton(u, "Click here to save this patch."),
			removeButton(u, "Click here to remove this patch."),

			patches(),

			searchBar(u, "Define a name or search for a patch!", "Init.."),
			patchesView(u, patches)
			//tagsSelector(u, patchList.getPatchList()),
			//inspector(u, patchList.getPatchList(), tagsSelector)
		{
			layout.init(
				{ 1, 3, 34, 13, 3, 3, 1 },
				{ 1, 3, 34, 21, 1 }
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
				savePatch();
			});

			searchBar.onReturn = saveButton.onClick.back();
			searchBar.onType = [&]()
			{
				filterEntries();
			};

			makeTextButton(removeButton, "rmv", false);
			removeButton.getLabel().textCID = ColourID::Abort;
			removeButton.onClick.push_back([&]()
			{
				removePatch();
			});

			addAndMakeVisible(closeButton);
			addAndMakeVisible(saveButton);
			addAndMakeVisible(removeButton);
			addAndMakeVisible(searchBar);
			addAndMakeVisible(patchesView);

			//addAndMakeVisible(tagsSelector);
			//addAndMakeVisible(inspector);

			onScreenshotFX.push_back([](Graphics& g, Image& img)
				{
					imgPP::blur(img, g, 7);

					auto bgCol = Colours::c(ColourID::Bg);

					if (bgCol.getPerceivedBrightness() < .5f)
						for (auto y = 0; y < img.getHeight(); ++y)
							for (auto x = 0; x < img.getWidth(); ++x)
								img.setPixelAt
								(x, y,
									img.getPixelAt(x, y)
									.withMultipliedSaturation(.4f)
									.withMultipliedBrightness(.4f)
								);
					else
						for (auto y = 0; y < img.getHeight(); ++y)
							for (auto x = 0; x < img.getWidth(); ++x)
								img.setPixelAt
								(x, y,
									img.getPixelAt(x, y)
									.withMultipliedSaturation(.4f)
									.withMultipliedBrightness(1.5f)
								);
				});

#if DebugNumPatches != 0
			Random rand;
			for (auto i = 0; i < DebugNumPatches; ++i)
			{
				String strA;
				appendRandomString(strA, rand, 12);
				searchBar.setText(strA);
				savePatch();
			}
#endif
		}

		void setVisible(bool e) override
		{
			if (e)
			{
				notify(EvtType::BrowserOpened);
				takeScreenshot();
				Comp::setVisible(e);
				searchBar.enable();
			}
			else
			{
				notify(EvtType::BrowserClosed);
				searchBar.disable();
				Comp::setVisible(e);
			}
		}

		void paint(Graphics& g) override
		{
			CompScreenshotable::paint(g);
		}

		void resized() override
		{
			CompScreenshotable::resized();

			layout.resized();

			layout.place(closeButton, 1, 1, 1, 1, true);
			layout.place(saveButton, 4, 1, 1, 1, true);
			layout.place(removeButton, 5, 1, 1, 1, true);
			layout.place(searchBar, 2, 1, 2, 1, false);
			layout.place(patchesView, 1, 2, 2, 2, false);

			//layout.place(tagsSelector, 2, 2, 1, 1, false);
			//layout.place(inspector, 1, 4, 4, 1, false);

		}

		String getSelectedPatchName()
		{
			const auto patch = patches.getSelected();
			if (patch != nullptr)
				return patch->name;
			return "init";
		}

	protected:
		Button closeButton;
		Button saveButton, removeButton;

		Patches patches;

		TextEditor searchBar;
		PatchesViewSortable patchesView;
		//TagsSelector tagsSelector;
		//PatchInspector inspector;

		void savePatch()
		{
			if (patches.save(searchBar.getText(), "user"))
			{
				searchBar.clear();

				auto& view = patchesView.addView();
				view.onClick.push_back([&]()
				{
					updateSelection();
				});

				updateSelection();
				filterEntries();
			}
		}

		void removePatch()
		{
			const auto selected = patches.getSelected();

			if (!patchesView.remove(selected))
				return;

			patches.removeSelected();

			patchesView.resized();
			updateSelection();
		}

		void updateSelection()
		{
			auto selected = patches.getSelected();

			patchesView.select(selected);

			repaintWithChildren(this);
		}

		void filterEntries()
		{
			const auto& searchBarText = searchBar.getText().toLowerCase();

			patchesView.filterEntries(searchBarText);

			patchesView.resized();
			repaintWithChildren(this);
		}
	};
	*/

// OLD STUFF END

namespace gui
{
	static constexpr int PatchNameWidth = 13;
	static constexpr int PatchAuthorWidth = 8;
	
	struct Patch :
		public Button
	{
		Patch(Utils& u, const String& _name, const String& _author) :
			Button(u, "Click on this patch in order to select it."),
			name(u, _name),
			author(u, _author),
			tags()
		{
			layout.init(
				{ 1, PatchNameWidth, PatchAuthorWidth, 1 },
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

		bool operator==(const Patch& other) const noexcept
		{
			return other.name.getText() == name.getText()
				&& other.author.getText() == author.getText();
		}

		bool operator!=(const Patch& other) const noexcept
		{
			return !this->operator==(other);
		}

		bool isRemovable() const
		{
			return author.getText() != "factory";
		}

		void resized() override
		{
			layout.resized();

			layout.place(name, 1, 0, 1, 1, false);
			layout.place(author, 2, 0, 1, 1, false);
		}

		Label name, author;
		std::vector<String> tags;
	};

	static constexpr float PatchRelHeight = 8.f;

	struct Patches :
		public CompScrollable
	{
		using UniquePatch = std::unique_ptr<Patch>;
		using SortFunc = std::function<bool(const UniquePatch&, const UniquePatch&)>;

		Patches(Utils& u) :
			CompScrollable(u),
			patches(),
			listBounds()
		{
			layout.init(
				{ 34, 1 },
				{ 1 }
			);
		}

		int getIdx(const Patch& nPatch) const noexcept
		{
			for (auto i = 0; i < numPatches(); ++i)
				if (nPatch == *patches[i])
					return i;
			return -1;
		}

		bool contains(const Patch& nPatch) const noexcept
		{
			return getIdx(nPatch) != -1;
		}

		bool save(const String& name, const String& author)
		{
			if (name.isEmpty() || author.isEmpty())
				return false;

			Patch nPatch(utils, name, author);

			if (contains(nPatch))
				return false;

			patches.push_back(std::make_unique<Patch>(utils, name, author));
			auto& patch = *patches.back();
			
			patch.onClick.push_back([&ts = patch.toggleState, &list = patches]()
			{
				for (auto& p : list)
					p->toggleState = 0;
					
				ts = 1;
			});
			patch.onClick.push_back([this]()
			{
				resized();
				repaintWithChildren(this);
			});

			addAndMakeVisible(patch);
			select(&patch);
			
			resized();
			repaintWithChildren(this);

			return true;
		}

		bool removeSelected()
		{
			auto selected = getSelected();

			if(selected != nullptr && selected->isRemovable())
				for (auto i = 0; i < patches.size(); ++i)
					if (*patches[i] == *selected)
					{
						select(nullptr);

						patches.erase(patches.begin() + i);
						
						resized();
						repaintWithChildren(this);

						return true;
					}
			return false;
		}

		bool select(Patch* nPatch) noexcept
		{
			for (auto& p : patches)
				p->toggleState = 0;

			if (nPatch == nullptr)
				return false;

			for (auto& ptchs : patches)
				if (*ptchs == *nPatch)
				{
					nPatch->toggleState = 1;
					return true;
				}

			return false;
		}

		int getSelectedIdx() const noexcept
		{
			for (auto i = 0; i < numPatches(); ++i)
				if (patches[i]->toggleState == 1)
					return i;
			return -1;
		}

		const Patch* getSelected() const noexcept
		{
			auto idx = getSelectedIdx();
			if (idx == -1)
				return nullptr;
			return patches[idx].get();
		}

		Patch& back() noexcept { return *patches.back(); }

		const Patch& back() const noexcept { return *patches.back(); }

		Patch& operator[](int i) noexcept { return *patches[i]; }

		const Patch& operator[](int i) const noexcept { return *patches[i]; }

		size_t numPatches() const noexcept { return patches.size(); }

		void sort(const SortFunc& sortFunc)
		{
			std::sort(patches.begin(), patches.end(), sortFunc);
			
			resized();
			repaintWithChildren(this);
		}

		void resized() override
		{
			layout.resized();

			layout.place(scrollBar, 1, 0, 1, 1);

			listBounds = layout(0, 0, 1, 1);

			const auto x = listBounds.getX();
			const auto w = listBounds.getWidth();
			const auto h = utils.thicc * PatchRelHeight;
			actualHeight = h * static_cast<float>(numPatches());

			auto y = listBounds.getY() - yScrollOffset;

			for (auto p = 0; p < numPatches(); ++p)
			{
				auto& patch = *patches[p];

				if (patch.isVisible())
				{
					patch.setBounds(BoundsF(x, y, w, h).toNearestInt());
					y += h;
				}
			}
		}

		void applyFilters(const String& text)
		{
			if (text.isEmpty())
			{
				for (auto& patch : patches)
					patch->setVisible(true);
			}
			else
			{
				const auto txt = text.toLowerCase();

				for (auto& patch : patches)
					patch->setVisible(patch->name.getText().toLowerCase().contains(txt));
			}
			
			resized();
			repaintWithChildren(this);
		}

	protected:
		std::vector<UniquePatch> patches;
		BoundsF listBounds;

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
			auto y = listBounds.getY() - yScrollOffset;
			auto w = listBounds.getWidth();
			auto btm = listBounds.getBottom();
			auto r = utils.thicc * PatchRelHeight;

			g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
			for (auto i = 0; i < numPatches(); ++i)
			{
				if (y >= btm)
					return;
				if (i % 2 == 0)
					g.fillRect(x, y, w, r);

				y += r;
			}
		}
	};

	struct PatchesSortable :
		public Comp
	{
		using UniquePatch = Patches::UniquePatch;
		using SortFunc = Patches::SortFunc;

		PatchesSortable(Utils& u) :
			Comp(u, "", CursorType::Default),
			patches(u),
			sortByName(u, "Click here to sort all patches by name."),
			sortByAuthor(u, "Click here to sort all patches by author.")
		{
			layout.init(
				{ 21, 1 },
				{ 2, 34 }
			);

			addAndMakeVisible(sortByName);
			addAndMakeVisible(sortByAuthor);

			makeTextButton(sortByName, "NAME");
			makeTextButton(sortByAuthor, "AUTHOR");

			sortByName.onClick.push_back([&]()
				{
					sortByName.toggleState = sortByName.toggleState == 0 ? 1 : 0;

					SortFunc sortFunc = [&ts = sortByName.toggleState](const UniquePatch& a, const UniquePatch& b)
					{
						const auto& pA = a->name.getText();
						const auto& pB = b->name.getText();

						if (ts == 1)
							return pA.compareNatural(pB) > 0;
						else
							return pA.compareNatural(pB) < 0;
					};

					sort(sortFunc);
				});

			sortByAuthor.onClick.push_back([&]()
				{
					sortByAuthor.toggleState = sortByAuthor.toggleState == 0 ? 1 : 0;

					SortFunc sortFunc = [&ts = sortByAuthor.toggleState](const UniquePatch& a, const UniquePatch& b)
					{
						const auto& pA = a->author.getText();
						const auto& pB = b->author.getText();

						if (ts == 1)
							return pA.compareNatural(pB) > 0;
						else
							return pA.compareNatural(pB) < 0;
					};

					sort(sortFunc);
				});

			{
				auto& nLabel = sortByName.getLabel();
				auto& authLabel = sortByAuthor.getLabel();

				nLabel.textCID = ColourID::Txt;
				authLabel.textCID = nLabel.textCID;

				nLabel.font = getFontDosisMedium();
				authLabel.font = nLabel.font;

				nLabel.just = Just::centredLeft;
				authLabel.just = nLabel.just;

				nLabel.mode = Label::Mode::TextToLabelBounds;
				authLabel.mode = nLabel.mode;
			}

			addAndMakeVisible(patches);
		}

		int getIdx(const Patch& nPatch) const noexcept
		{
			return patches.getIdx(nPatch);
		}

		bool contains(const Patch& nPatch) const noexcept
		{
			return patches.contains(nPatch);
		}

		bool save(const String& name, const String& author)
		{
			return patches.save(name, author);
		}

		bool removeSelected()
		{
			return patches.removeSelected();
		}

		bool select(Patch* nPatch) noexcept
		{
			return patches.select(nPatch);
		}

		int getSelectedIdx() const noexcept
		{
			return patches.getSelectedIdx();
		}

		const Patch* getSelected() const noexcept
		{
			return patches.getSelected();
		}

		Patch& back() noexcept { return patches.back(); }

		const Patch& back() const noexcept { return patches.back(); }

		Patch& operator[](int i) noexcept { return patches[i]; }

		const Patch& operator[](int i) const noexcept { return patches[i]; }

		size_t numPatches() const noexcept { return patches.numPatches(); }

		void sort(const SortFunc& sortFunc)
		{
			patches.sort(sortFunc);
		}

		void resized() override
		{
			layout.resized();

			auto buttonArea = layout(0, 0, 1, 1);
			{
				auto x = buttonArea.getX();
				auto y = buttonArea.getY();
				auto w = buttonArea.getWidth();
				auto h = buttonArea.getHeight();

				const auto bothWidth = 1.f / static_cast<float>(PatchNameWidth + PatchAuthorWidth);
				const auto nameW = w * static_cast<float>(PatchNameWidth) * bothWidth;
				const auto authorW = w * static_cast<float>(PatchAuthorWidth) * bothWidth;

				sortByName.setBounds(BoundsF(x, y, nameW, h).toNearestInt());
				x += nameW;
				sortByAuthor.setBounds(BoundsF(x, y, authorW, h).toNearestInt());
			}

			layout.place(patches, 0, 1, 2, 1, false);
		}

		void applyFilters(const String& text)
		{
			patches.applyFilters(text);
		}

		Patches& getPatches() noexcept { return patches; }

		const Patches& getPatches() const noexcept { return patches; }

	protected:
		Patches patches;
		Button sortByName, sortByAuthor;

		void paint(Graphics&) override {}
	};

	static constexpr float TagRelHeight = 8.f;

	struct TagsSelector :
		public CompScrollable
	{
		struct Tag :
			public Button
		{
			Tag(Utils& u, const String& str = "") :
				Button(u, "Click on this tag to (de-)select it!"),
				refCount(1)
			{
				makeTextButton(*this, str);
			}

			std::atomic<int> refCount;
		};

		using UniqueTag = std::unique_ptr<Tag>;

		TagsSelector(Utils& u, Patches& patches) :
			CompScrollable(u),
			tags()
		{
			layout.init(
				{ 21, 1 },
				{ 1 }
			);

			for (auto i = 0; i < patches.numPatches(); ++i)
			{
				const auto& patch = patches[i];
				for (const auto& tag : patch.tags)
					addTag(tag);
			}
		}

		int getIdx(const String& txt) const noexcept
		{
			for (auto i = 0; i < numTags(); ++i)
			{
				const auto& tag = tags[i]->getText();
				if (tag == txt)
					return i;
			}
			return -1;
		}

		bool tagExists(const String& txt) const noexcept
		{
			const auto idx = getIdx(txt);
			return idx == -1 ? false : true;
		}

		bool addTag(const String& txt)
		{
			const auto idx = getIdx(txt);

			if (idx != -1)
			{
				// tag already exist
				++tags[idx]->refCount;
				return false;
			}

			tags.push_back(std::make_unique<Tag>(
				utils, txt
			));
			auto& tag = *tags.back();
			addAndMakeVisible(tag);

			resized();
			repaintWithChildren(this);

			return true;
		}

		bool removeTag(const String& txt)
		{
			const auto idx = getIdx(txt);
			if (idx == -1)
			{
				// tag doesn't exist
				return false;
			}

			auto& tag = *tags[idx];
			const auto count = tag.refCount.load() - 1;
			if (count == 0)
			{
				// remove tag
				tags.erase(tags.begin() + idx);

				resized();
				repaintWithChildren(this);

				return true;
			}

			// tag still exists
			tag.refCount.store(count);

			return false;
		}

		void paint(Graphics&) override {}

		void resized() override
		{
			layout.resized();

			layout.place(scrollBar, 1, 0, 1, 1, false);

			const auto bounds = layout(0, 0, 1, 1, false);

			const auto width = bounds.getWidth();
			const auto right = bounds.getRight();

			const auto thicc = utils.thicc;

			auto x = bounds.getX();
			auto y = bounds.getY() - yScrollOffset;
			const auto h = TagRelHeight * thicc;
			const auto w = h * 3.f;

			const auto numTgs = static_cast<float>(numTags());
			const auto numTagsPerRow = std::floor(width / w);
			actualHeight = numTgs * h / numTagsPerRow;

			for (auto& tag : tags)
			{
				if (x + w > right)
				{
					x = 0.f;
					y += h;
				}

				tag->setBounds(BoundsF(x, y, w, h).toNearestInt());

				x += w;
			}
		}

		size_t numTags() const noexcept
		{
			return tags.size();
		}

		std::vector<UniqueTag> tags;
	};


	struct PatchBrowser :
		public CompScreenshotable
	{
		PatchBrowser(Utils& u) :
			CompScreenshotable(u),

			closeButton(u, "Click here to close the browser."),
			saveButton(u, "Click here to save this patch."),
			removeButton(u, "Click here to remove this patch."),

			patches(u),

			searchBar(u, "Define a name or search for a patch!", "Init.."),
			tagsSelector(u, patches.getPatches())
			//inspector(u, patchList.getPatchList(), tagsSelector)
		{
			layout.init(
				{ 1, 3, 34, 13, 3, 3, 1 },
				{ 1, 3, 34, 21, 1 }
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
				savePatch();
			});

			searchBar.onReturn = saveButton.onClick.back();
			searchBar.onType = [&]()
			{
				applyFilters();
			};

			makeTextButton(removeButton, "rmv", false);
			removeButton.getLabel().textCID = ColourID::Abort;
			removeButton.onClick.push_back([&]()
			{
				removePatch();
			});

			addAndMakeVisible(closeButton);
			addAndMakeVisible(saveButton);
			addAndMakeVisible(removeButton);
			addAndMakeVisible(searchBar);
			addAndMakeVisible(patches);
			addAndMakeVisible(tagsSelector);
			//addAndMakeVisible(inspector);

			onScreenshotFX.push_back([](Graphics& g, Image& img)
				{
					imgPP::blur(img, g, 7);

					auto bgCol = Colours::c(ColourID::Bg);

					if (bgCol.getPerceivedBrightness() < .5f)
						for (auto y = 0; y < img.getHeight(); ++y)
							for (auto x = 0; x < img.getWidth(); ++x)
								img.setPixelAt
								(x, y,
									img.getPixelAt(x, y)
									.withMultipliedSaturation(.4f)
									.withMultipliedBrightness(.4f)
								);
					else
						for (auto y = 0; y < img.getHeight(); ++y)
							for (auto x = 0; x < img.getWidth(); ++x)
								img.setPixelAt
								(x, y,
									img.getPixelAt(x, y)
									.withMultipliedSaturation(.4f)
									.withMultipliedBrightness(1.5f)
								);
				});

#if DebugNumPatches != 0
			Random rand;
			for (auto i = 0; i < DebugNumPatches; ++i)
			{
				String strA;
				appendRandomString(strA, rand, 12);
				searchBar.setText(strA);
				savePatch();
			}
#endif
		}

		void setVisible(bool e) override
		{
			if (e)
			{
				notify(EvtType::BrowserOpened);
				takeScreenshot();
				Comp::setVisible(e);
				searchBar.enable();
			}
			else
			{
				notify(EvtType::BrowserClosed);
				searchBar.disable();
				Comp::setVisible(e);
			}
		}

		void paint(Graphics& g) override
		{
			CompScreenshotable::paint(g);
		}

		void resized() override
		{
			CompScreenshotable::resized();

			layout.resized();

			layout.place(closeButton, 1, 1, 1, 1, true);
			layout.place(saveButton, 4, 1, 1, 1, true);
			layout.place(removeButton, 5, 1, 1, 1, true);
			layout.place(searchBar, 2, 1, 2, 1, false);
			layout.place(patches, 1, 2, 2, 2, false);
			layout.place(tagsSelector, 2, 2, 1, 1, false);
			//layout.place(inspector, 1, 4, 4, 1, false);

		}

		String getSelectedPatchName() const
		{
			const auto patch = patches.getSelected();
			if (patch != nullptr)
				return patch->name.getText();
			return "init";
		}

	protected:
		Button closeButton;
		Button saveButton, removeButton;

		PatchesSortable patches;

		TextEditor searchBar;
		TagsSelector tagsSelector;
		//PatchInspector inspector;

		void savePatch()
		{
			const auto& str = searchBar.getText();

			patches.save(str, "user");

			searchBar.clear();
			applyFilters();
		}

		void removePatch()
		{
			patches.removeSelected();
		}

		void applyFilters()
		{
			const auto& str = searchBar.getText();

			patches.applyFilters(str);
		}
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
					if (e)
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

#undef DebugNumPatches