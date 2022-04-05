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

Options Menu
	implement more sub menu types
	think about how to make automatic tutorials or manuals
	how to make automatic updates possible

Param
	bias curve of max modulation depth
	locking max modulation depth
	saving and loading max mod depth as preset

Oversampler
	less cpu demanding (FFT?)

Design
	deliberate x,y offset to label border for pseudo 3d fx

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
	Options menu
	Parameter Randomizer
	Preset Browser
	undo/redo
	delta button
	patches select parameter interpolation or parallel processing of sound
		can highlevel params be macro-modulated in parallel? sketch out block diagram!
	sidechain activated?

Meters
	...

Knob
	combine Knob and Dial into one with Parametr
	make templated (or so)
	right click for context menu
		- midi learn
		- save as default value

-------------------------------------------------------------------

FEATURE IDEAS:

Achievements System?
	discuss relevance, pros and cons

*/