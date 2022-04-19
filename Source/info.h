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

Solve JUCE Bugs
	Graphics::drawFittedText skips first line if it only has space or \n
		seen in options/controls' text

MIDI Learn
	automation doesnt work in bitwig
	1 cc to n parameters
	different midi channnels (16 * 128 CCs instead of 128)
	possible to save and load default state of whole midi learn patch

PopUp
	MIDI Learn
	clear MIDI Learn
	enter value

Label
	group has bad architecture. doesn't go to right value on first resized
	consider text changes as source of setHeight as well

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
	Jedes Patch hat eigenes Farbschema?
	Name?
	Preset Browser pro Patch oder pro Sub-Patch?

PPD
	PPDSidechainable

HighLevel UI Elements
	Parameter Randomizer
	Preset Browser
	undo/redo
	delta button
	sidechain activated?

Meters
	...

Knob
	combine Knob and Dial into one with Parametr
	make templated (or so)

-------------------------------------------------------------------

FEATURE IDEAS:

Achievements System?
	discuss relevance, pros and cons

*/