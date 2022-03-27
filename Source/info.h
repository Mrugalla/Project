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
4. ...


-------------------------------------------------------------------

TO DO:

Button
	slightly rough lines when non-rect hitbox

Design
	deliberate x,y offset to label border for pseudo 3d fx

FirstTimeAction
	calculate lookup tables for hitboxes

ValueBubble
	not needed anymore. value shown in parameter labels.
	remove correspndg parameter changed events
	add label to dial

ProcessorBase
	...

LowLevel UI Elements
	Jedes Patch hat eigenes Farbschema
	Name?
	Preset Browser

PPD
	PPDSidechainable

HighLevel UI Elements
	Options menu
	Parameter Randomizer
	Preset Browser
	undo/redo
	lookahead needed?
	delta button
	patches select parameter interpolation or parallel processing of sound
	sidechain activated?

Meters
	...

Knob
	combine Knob and Dial into one with Parametr
	make templated (or so)
	right click for context menu
		- randomize
		- midi learn
		- set to default value
		- save as default value
		- lock

-------------------------------------------------------------------

FEATURE IDEAS:

Achievements System?
	discuss relevance, pros and cons

*/