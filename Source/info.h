/*

-------------------------------------------------------------------

Welcome to DEFAULT PROJECT!

This project contains my vision of a default JUCE VST plugin project. I made this so I can get started trying new ideas
and finishing projects way faster than from a projucer-made default project.

-------------------------------------------------------------------

HOW TO USE:

1. Copy the folder "Source" as well as the file "Project.jucer" into a new folder.
2. In Project.jucer
	2.1 Rename project name
	2.2 Rename plugin name
	2.3 Define unique plugin ID
3. Open config.h and define the needed pro processor definitions
4. Open LowLevel.h and define all low level parameters, layout and stuff
5. Ship it!


-------------------------------------------------------------------

TO DO:

FormularParser
	make more lightweight
		maybe remove message string
		values push_backs in calculate and calculateX needed?

Param
	...

ParameterRandomizer
	can randomize power param. better make buttons lockable!

Comp
	rewrite all components to not have makeNotify methods anymore but making stuff in constructor

TextEditor
	fast repeated "back" hits not registered

sta::State
	contains valueTree as well as ApplicationProperties for max workflow!
	implement undo/redo

MIDI Learn
	setValueNotifyHost() uses ScopedLock! needs message thread?
	1 cc to n parameters?
	different midi channnels (16 * 128 CCs instead of 128)
	possible to save and load default state of whole midi learn patch

PopUp
	implement context menus for
		Buttons
		ccMonitor

Label
	make free func(s) for grouping labels (n buttons)

Options Menu
	lookandfeel thickness
	think about how to make automatic tutorials or manuals
	how to make automatic updates possible

All Params
	saving and loading max mod depths as preset
	setting max mod depths as absolute

Param
	...

Oversampler
	less cpu demanding (FFT?)

Tutorial at first open

FirstTimeAction
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