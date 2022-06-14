#pragma once
#include "Button.h"
#include "GUIParams.h"

namespace gui
{
	struct TextEditor :
		public Comp,
		public Timer
	{
		TextEditor(Utils&, const String&/*tooltip*/, Notify&&, const String &/*empty string*/ = "enter value..");

		TextEditor(Utils&, const String&, const String&);

		void setVisible(bool) override;

		void enable();

		bool isEnabled() const noexcept;

		void disable();

		const String& getText() const noexcept;

		void setText(const String&);

		bool isEmpty() const noexcept;

		bool isNotEmpty() const noexcept;

		void clear();

		std::function<void()> onEscape, onReturn, onType, onRemove;
	protected:
		Label label;
		String emptyString, txt;
		BlinkyBoy blinkyBoy;
		int tickIdx;
		bool drawTick;

		void mouseUp(const Mouse&) override;

		void paint(Graphics&) override;

		void resized() override;

		void updateLabel();

		void timerCallback() override;

		bool keyPressed(const KeyPress&) override;
	};

	struct TextEditorKnobs :
		public TextEditor
	{
		Notify makeNotify(TextEditorKnobs&);

		TextEditorKnobs(Utils&);

		void paint(Graphics&) override;
	};
}

