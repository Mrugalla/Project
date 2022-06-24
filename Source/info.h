/*

-------------------------------------------------------------------

Welcome to DEFAULT PROJECT!

This project contains my vision of a default JUCE VST plugin project. I made this so I can get started trying new ideas
and finishing projects way faster than from a projucer-made template.

-------------------------------------------------------------------

HOW TO USE:

1. Copy the folder "Source" as well as the file "Project.jucer" into a new folder.
2. In Project.jucer
	2.1 Rename plugin name
	2.2 Define unique plugin ID
	2.3 Configurate the pre processor definitions
4. Open LowLevel.h and design all low level parameters, layout and stuff
5. Ship it!


-------------------------------------------------------------------

TO DO:

Label
	TextToLabelBounds accounting for multiline text
		size of text
		where click in texteditor makes tick appear

Knob2
	test if parameter locking works!
		randomize
	replace functionality in context menu

SplineEditor
	make points come from outside (maybe?)
	wavetable from spline

Macro Dropdown
	Save All MaxModDepths and Bias-Patch
	Load Some MaxModDepths and Bias-Patch (FileChooser?)
	Switch: Control MaxModDepth or Bias
	Switch: Rel/Abs MaxModDepth
	Flip Parameter Value with MaxModDepth
	Remove All MaxModDepth and Bias

Colours
	throw some colours out of the options menu
	like Transparent or Darken

Knob & Scrollbar
	..

Shader
	..

ScrollBar
	some things still not handled by scrollbarcomp alone
	enable scrolling not only on scrollbar hover

PatchBrowser
	revisit tag system

FormularParser
	is log and ln working right?
	make more lightweight
		values push_backs in calculate and calculateX needed?
		make callbacks instead of buffers?

Param
	...

ParameterRandomizer
	...

Comp
	make cpp of stuff

TextEditor
	minimize > back > click > tick doesn't move > 2nd click > does move

sta::State
	contains valueTree as well as ApplicationProperties for max workflow!
	implement undo/redo

MIDI Learn
	1 cc to n parameters?
	different midi channnels (16 * 128 CCs instead of 128)
	possible to save and load default state of whole midi learn patch

ScrollBar
	have own header/cpp

ContextMenu
	implement context menu(s) for
		ccMonitor

Label
	make free func(s) for grouping labels (n buttons)

Options Menu
	think about how to make automatic tutorials or manuals
	how to make automatic updates possible

All Params
	saving and loading max mod depths as preset

Param
	...

Oversampler
	make less cpu demanding (FFT?)

FirstTimeAction(s)
	thing that only makes lookuptables when the plugin is started the first time
	tutorial

ProcessorBase
	...

LowLevel UI Elements
	...

Pre Processor Defines (PPD)
	PPDSidechainable

HighLevel UI Elements
	undo/redo buttons
	sidechain activated?

Meters
	...

Knob
	...

-------------------------------------------------------------------

FEATURE IDEAS:

Achievements System?
	discuss relevance, pros and cons

*/