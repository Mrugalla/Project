#pragma once
#include "Comp.h"
#include "Shared.h"

namespace gui
{
	using ValueTree = juce::ValueTree;
	using Identifier = juce::Identifier;

	inline Just getJust(const String& t)
	{
		if (t == "left") return Just::left;
		if (t == "top") return Just::top;
		if (t == "bottom") return Just::bottom;
		if (t == "right") return Just::right;
		
		return Just::centred;
	}

	struct ComponentWithBounds
	{
		template<typename CompType>
		ComponentWithBounds(CompType* _c = nullptr, BoundsF&& _b = BoundsF(0.f, 0.f, 1.f, 1.f), bool _isQuad = false) :
			c(_c == nullptr ? nullptr : _c),
			b(_b),
			isQuad(_isQuad)
		{}

		std::unique_ptr<Component> c;
		BoundsF b;
		bool isQuad;
	};

	struct CompModular :
		public Comp
	{
		CompModular(Utils& u, String&& _tooltip, CursorType ct) :
			Comp(u, std::move(_tooltip), ct)
		{}

		void init()
		{
			for (auto& cmp : comps)
				addAndMakeVisible(*cmp.c);
		}

		std::vector<ComponentWithBounds> comps;
	protected:
		void paint(Graphics& g) override
		{
			Comp::paint(g);
			layout.paint(g);
		}

		void resized() override
		{
			layout.resized();

			for (auto& cmp : comps)
				if(cmp.c != nullptr)
					layout.place(
						*cmp.c,
						cmp.b.getX(),
						cmp.b.getY(),
						cmp.b.getWidth(),
						cmp.b.getHeight(),
						cmp.isQuad
					);
		}
	};

	class NavBar :
		public Comp
	{
		struct Node
		{
			Node(const ValueTree& _vt, int _x, int _y) :
				vt(_vt),
				x(_x),
				y(_y)
			{}

			const ValueTree vt;
			const int x, y;
		};

		using Nodes = std::vector<Node>;

		Nodes makeNodes(const ValueTree& xml, int x = 0, int y = 0)
		{
			Nodes ndes;

			for (auto i = 0; i < xml.getNumChildren(); ++i)
			{
				const auto child = xml.getChild(i);
				if (child.hasType("menu"))
				{
					ndes.push_back({ child, x, y });
					
					const auto moarNodes = makeNodes(child, x + 1, y + 1);
					for (const auto& n : moarNodes)
						ndes.push_back(n);
					y = ndes.back().y + 1;
				}
			}

			return ndes;
		}

		int getDeepestNode() const noexcept
		{
			int d = 0;
			for (const auto& n : nodes)
				if (n.x > d)
					d = n.x;
			return d;
		}

	public:
		NavBar(Utils& u, const ValueTree& xml) :
			Comp(u, "", CursorType::Default),
			label(u, "< Nav >"),
			nodes(makeNodes(xml)),
			buttons(),
			numMenus(nodes.size()),
			deepestNode(getDeepestNode())
		{
			label.setTooltip("Click on a node in order to navigate to its sub menu.");

			std::vector<int> a, b;
			a.resize(numMenus + 1, 1);
			b.resize(deepestNode + 1, 1);

			layout.init(b, a);
			
			addAndMakeVisible(label);

			buttons.reserve(numMenus);
			for (auto i = 0; i < numMenus; ++i)
			{
				const auto& node = nodes[i].vt;

				buttons.emplace_back(std::make_unique<Button>(
					utils, node.getProperty("tooltip").toString()
				));

				auto& btn = *buttons[i];

				makeTextButton(btn, "- " + node.getProperty("id").toString(), true, true, 1);
				btn.getLabel().just = Just::left;
				btn.getLabel().font = Font();
			}

			makeButtonsGroup(buttons, 0);

			for (auto& btn : buttons)
				addAndMakeVisible(*btn);
		}

		void init(std::unique_ptr<CompModular>& subMenu, Comp& parent)
		{
			for (auto i = 0; i < numMenus; ++i)
			{
				auto& btn = *buttons[i];

				// make navigation functionality

				btn.onClick.push_back([&sub = subMenu, &prnt = parent, &node = nodes[i]]()
					{
						auto& utils = prnt.getUtils();

						sub.reset(new CompModular(utils, "", CursorType::Default));

						auto& comps = sub->comps;

						{
							const auto& xLayoutProp = node.vt.getProperty("x");
							const auto& yLayoutProp = node.vt.getProperty("y");
							if (xLayoutProp.isUndefined() || yLayoutProp.isUndefined())
								return;

							sub->initLayout(xLayoutProp.toString(), yLayoutProp.toString());
						}

						enum Type{ kTitle, kTxt, kNumTypes };
						std::array<Identifier, kNumTypes> ids
						{
							"title",
							"txt"
						};

						for (auto c = 0; c < node.vt.getNumChildren(); ++c)
						{
							const auto child = node.vt.getChild(c);

							const auto& xProp = child.getProperty("x", 0.f);
							const auto& yProp = child.getProperty("y", 0.f);
							const auto& wProp = child.getProperty("w", 1.f);
							const auto& hProp = child.getProperty("h", 1.f);

							Component* comp{ nullptr };

							if (child.getType() == ids[kTitle])
							{
								auto cmp = new Label(utils, node.vt.getProperty("id").toString());
								
								comp = cmp;
							}
							else if (child.getType() == ids[kTxt])
							{
								auto cmp = new Label(utils, child.getProperty("text").toString());
								cmp->just = getJust(child.getProperty("just").toString());
								cmp->font = juce::Font();

								comp = cmp;
							}

							if(comp != nullptr)
								comps.push_back(ComponentWithBounds({
										comp,
										{
											static_cast<float>(xProp),
											static_cast<float>(yProp),
											static_cast<float>(wProp),
											static_cast<float>(hProp)
										},
										false
									}));
						}

						sub->init();

						prnt.addAndMakeVisible(*sub);
						prnt.getLayout().place(*sub, 1, 2, 2, 1, false);
					});
			}

			for (auto& oc : buttons.front()->onClick)
				oc();
		}

	protected:
		Label label;
		Nodes nodes;
		std::vector<std::unique_ptr<Button>> buttons;
		const int numMenus, deepestNode;

		void paint(Graphics&) override
		{
			//Comp::paint(g);
			//layout.paint(g);
		}

		void resized() override
		{
			layout.resized();

			layout.place(label, 0, 0, deepestNode + 1, 1, false);

			if (numMenus > 0)
			{
				for (auto i = 0; i < numMenus; ++i)
				{
					auto& btn = *buttons[i];
					const auto& node = nodes[i];

					layout.place(btn,
						0.f + .5f * node.x,
						1 + node.y,
						1.f + deepestNode - .5f * node.x,
						1,
						false
					);
				}
			}
		}
	};

	struct Menu :
		public CompWidgetable
	{
		Menu(Utils& u, const ValueTree& xml) :
			CompWidgetable(u, "", CursorType::Default),
			label(u, xml.getProperty("id", "")),
			navBar(u, xml),
			subMenu(nullptr)
		{
			layout.init(
				{ 20, 50, 20 },
				{ 20, 50, 750, 20 }
			);

			addAndMakeVisible(label);
			addAndMakeVisible(navBar);

			navBar.init(subMenu, *this);
		}

	protected:
		Label label;
		NavBar navBar;
		std::unique_ptr<CompModular> subMenu;

		void paint(juce::Graphics& g) override
		{
			g.fillAll(Colours::c(ColourID::Darken));
		}

		void resized() override
		{
			layout.resized();

			layout.place(label, 1, 1, 1, 1, false);
			layout.place(navBar, 0, 2, 1, 1, false);
			if (subMenu != nullptr)
				layout.place(*subMenu, 1, 2, 2, 1, false);

		}
	};
}