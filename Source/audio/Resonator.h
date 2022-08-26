#pragma once
#include <array>
#include "WHead.h"
#include "AudioUtils.h"
#include "../arch/Interpolation.h"
#include "PRM.h"
#include "MIDIManager.h"

namespace audio
{
	
	struct DelayFeedback
	{
		DelayFeedback() :
			ringBuffer(),
			size(0)
		{}

		void prepare(int _size)
		{
			size = _size;
			
			for (auto& rb : ringBuffer)
				rb.resize(size, 0.f);
		}
		
		/*
		samples, numChannels, numSamples,
		wHead, feedbackBuffer[-1,1], readHead
		*/
		void operator()(float** samples, int numChannels, int numSamples,
			const int* wHead, const float* fbBuf, const float** readHead) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				auto ring = ringBuffer[ch].data();
				const auto rHead = readHead[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = rHead[s];
					const auto fb = fbBuf[s];

					const auto sOut = interpolate::cubicHermiteSpline(ring, r, size);
					const auto sIn = smpls[s] + sOut * fb;

					ring[w] = sIn;
					smpls[s] = sOut;
				}
			}
		}
		
	protected:
		std::array<std::vector<float>, 2> ringBuffer;
		int size;
	};
	
	struct ResonatorVoice
	{
		ResonatorVoice() :
			delayFeedback(),
			audioBuffer(),
			readHead(),

			Fs(1.f),
			sizeF(0.f),
			curDelay(0.f),
			curNote(48)
		{}

		void prepare(float sampleRate, int blockSize, int size)
		{
			Fs = sampleRate;

			sizeF = static_cast<float>(size);
			
			delayFeedback.prepare(size);
			readHead.setSize(2, blockSize, false, false, false);
			audioBuffer.setSize(2, blockSize, false, false, false);

			curNote = 48; // C3
			const auto freqHz = noteInFreqHz(static_cast<float>(curNote), 12.f, 69.f, 440.f);
			curDelay = freqHzInSamples(freqHz, Fs);
		}

		void operator()(float** samples, int numChannels, int numSamples,
			const int* wHead, const float* fbBuf, const std::vector<MIDINote>& noteBuffer) noexcept
		{
			auto buffer = audioBuffer.getArrayOfWritePointers();
			{ // copy samples to parallel buffer
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto smpls = samples[ch];
					auto buf = buffer[ch];

					SIMD::copy(buf, smpls, numSamples);
				}
			}
			
			{ // calculate readhead indexes from note buffer
				auto rHeadBuf = readHead.getArrayOfWritePointers();

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto nNote = noteBuffer[s];

					if (curNote != nNote.noteNumber)
					{
						curNote = nNote.noteNumber;
						if (curNote != 0)
						{
							const auto freqHz = noteInFreqHz(static_cast<float>(curNote), 12.f, 69.f, 440.f);
							curDelay = freqHzInSamples(freqHz, Fs);
						}
					}

					const auto w = static_cast<float>(wHead[s]);
					auto r = w - curDelay;
					if (r < 0.f)
						r += sizeF;

					for (auto ch = 0; ch < numChannels; ++ch)
						rHeadBuf[ch][s] = r;
				}
			}
			
			const auto rHeadBufConst = readHead.getArrayOfReadPointers();

			delayFeedback(audioBuffer.getArrayOfWritePointers(), numChannels, numSamples,
				wHead, fbBuf, rHeadBufConst);
		}

		void addTo(float** samples, int numChannels, int numSamples) noexcept
		{
			auto buffer = audioBuffer.getArrayOfReadPointers();

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				const auto buf = buffer[ch];

				SIMD::copy(smpls, buf, numSamples);
			}
		}

		protected:
			DelayFeedback delayFeedback;
			AudioBuffer audioBuffer, readHead;

			float Fs, sizeF, curDelay;
			int curNote;
	};

	struct Resonator
	{
		Resonator(MIDIVoices& _midiVoices) :
			wHead(),
			midiVoices(_midiVoices),
			voices(),

			feedback(0.f),

			sizeF(0.f),
			size(0)
		{
		}

		void prepare(float sampleRate, int blockSize)
		{
			sizeF = freqHzInSamples(20.f, sampleRate);
			size = static_cast<int>(sizeF);

			wHead.prepare(blockSize, size);
			for(auto& voice: voices)
				voice.prepare(sampleRate, blockSize, size);

			feedback.prepare(sampleRate, blockSize, 10.f);
		}

		void operator()(float** samples, int numChannels, int numSamples,
			float _feedback) noexcept
		{
			wHead(numSamples);

			const auto fbBuf = feedback(_feedback, numSamples);
			for (auto v = 0; v < PPD_MIDINumVoices; ++v)
			{
				auto& voice = voices[v];
				const auto& midiVoice = midiVoices.voices[v];

				const auto wh = wHead.data();

				voice(samples, numChannels, numSamples, wh, fbBuf, midiVoice.buffer);
			}

			for (auto v = 0; v < PPD_MIDINumVoices; ++v)
				voices[v].addTo(samples, numChannels, numSamples);
		}

	protected:
		WHead wHead;
		const MIDIVoices& midiVoices;
		std::array<ResonatorVoice, PPD_MIDINumVoices> voices;
		
		PRM feedback;

		float sizeF;
		int size;
	};
}