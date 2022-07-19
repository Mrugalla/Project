#pragma once
#include "MIDILearn.h"
#include <functional>

namespace audio
{
	struct MIDIManager
	{
		MIDIManager(Params& params, State& state) :
			midiLearn(params, state),
			onInit(),
			onEnd(),
			onCC(),
			onNoteOn(),
			onNoteOff(),
			onNoEvt()
		{
			onInit.push_back([&learn = midiLearn](int)
			{
				learn.processBlockInit();
			});

			onCC.push_back([&learn = midiLearn](const MidiMessage& msg, int)
			{
				learn.processBlockMIDICC(msg);
			});

			onEnd.push_back([&learn = midiLearn](int)
			{
				learn.processBlockEnd();
			});
		}

		void savePatch()
		{
			midiLearn.savePatch();
		}

		void loadPatch()
		{
			midiLearn.loadPatch();
		}

		void operator()(MIDIBuffer& midi, int numSamples) noexcept
		{
			if (midi.isEmpty())
				processEmpty(numSamples);
			else
				processBlock(midi, numSamples);
		}

		MIDILearn midiLearn;

		std::vector<std::function<void(int/* numSamples */)>> onInit, onEnd;
		std::vector<std::function<void(const MidiMessage&, int/*sample index*/)>> onCC, onNoteOn, onNoteOff;
		std::vector<std::function<void(int/*sample index*/)>> onNoEvt;
	protected:

		void processEmpty(int numSamples) noexcept
		{
			for (auto& func : onInit)
				func(numSamples);
			for (auto& func : onEnd)
				func(numSamples);
		}

		void processBlock(MIDIBuffer& midi, int numSamples) noexcept
		{
			for (auto& func : onInit)
				func(numSamples);

			auto evt = midi.begin();
			auto ref = *evt;
			auto ts = ref.samplePosition;
			for (auto s = 0; s < numSamples; ++s)
			{
				if (ts > s)
				{
					for (auto& func : onNoEvt)
						func(s);
				}
				else
				{
					//bool noteOn = env.noteOn;
					while (ts == s)
					{
						const auto msg = ref.getMessage();

						if (msg.isNoteOn())
						{
							for (auto& func : onNoteOn)
								func(msg, s);
						}
						else if (msg.isNoteOff())
						{
							for (auto& func : onNoteOff)
								func(msg, s);
						}
						else if (msg.isPitchWheel())
						{
							//const auto pwv = msg.getPitchWheelValue();
							//pitchbendValue = static_cast<float>(pwv) * PBGain - 1.f;
							//currentValue = noteValue + pitchbendValue;
						}
						else if (msg.isController())
						{
							for (auto& func : onCC)
								func(msg, s);
						}
						++evt;
						if (evt == midi.end())
							ts = numSamples;
						else
						{
							ref = *evt;
							ts = ref.samplePosition;
						}
					}
					//bufNotes[s] = currentValue;
					//bufEnv[s] = env(noteOn) * SafetyCoeff;
				}
			}

			for (auto& func : onEnd)
				func(numSamples);
		}
	};

	struct MIDINote
	{
		float velocity;
		int noteNumber;
		bool noteOn;
	};

	struct MIDINoteBuffer
	{
		MIDINoteBuffer() :
			buffer(),
			curNote({ 0.f, 48, false }),
			sampleIdx(0)
		{
		}

		void prepare(int blockSize)
		{
			buffer.resize(blockSize, curNote);
		}
		
		void processNoteOn(const MIDINote& nNote, int ts) noexcept
		{
			for (auto s = sampleIdx; s < ts; ++s)
				buffer[s] = curNote;
			
			sampleIdx = ts;

			curNote = nNote;
			buffer[ts] = curNote;
		}

		void processNoteOff(int ts) noexcept
		{
			for (auto s = sampleIdx; s < ts; ++s)
				buffer[s] = curNote;
			
			sampleIdx = ts;

			curNote.noteOn = false;
			buffer[ts] = curNote;
		}

		void process(int numSamples) noexcept
		{
			for (auto s = sampleIdx; s < numSamples; ++s)
				buffer[s] = curNote;
			
			sampleIdx = numSamples;
		}

		std::vector<MIDINote> buffer;
		MIDINote curNote;
		int sampleIdx;
	};

	using MIDIVoicesArray = std::array<MIDINoteBuffer, PPD_MIDINumVoices>;

	struct MIDIVoices
	{
		MIDIVoices(MIDIManager& manager) :
			voices(),
			voiceIndex(0)
		{
			manager.onInit.push_back([this](int)
			{
				for (auto& voice : voices)
					voice.sampleIdx = 0;
			});
			manager.onNoteOn.push_back([this](const MidiMessage& msg, int s)
			{
				for (auto v = 1; v < PPD_MIDINumVoices; ++v)
				{
					auto nIdx = (voiceIndex + v) % PPD_MIDINumVoices;
					auto& voice = voices[voiceIndex];

					if (!voice.curNote.noteOn)
					{
						voiceIndex = nIdx;
						voice.processNoteOn(
							{
								msg.getFloatVelocity(),
								msg.getNoteNumber(),
								true
							},
							s
						);
						return;
					}
				}
				
				voiceIndex = (voiceIndex + 1) % PPD_MIDINumVoices;
				auto& voice = voices[voiceIndex];

				voice.processNoteOn(
					{
						msg.getFloatVelocity(),
						msg.getNoteNumber(),
						true
					},
					s
				);
			});
			manager.onNoteOff.push_back([this](const MidiMessage& msg, int s)
			{
				auto noteNumber = msg.getNoteNumber();
				for (auto v = 0; v < PPD_MIDINumVoices; ++v)
				{
					const auto v1 = (voiceIndex + 1 + v) % PPD_MIDINumVoices;

					auto& voice = voices[v1];

					if (voice.curNote.noteOn && voice.curNote.noteNumber == noteNumber)
						return voice.processNoteOff(s);
				}
			});
			manager.onEnd.push_back([this](int numSamples)
			{
				for (auto& voice : voices)
					voice.process(numSamples);
			});
		}

		void prepare(int blockSize)
		{
			for (auto& voice : voices)
				voice.prepare(blockSize);
		}
		
		MIDIVoicesArray voices;
		int voiceIndex;
	};
}