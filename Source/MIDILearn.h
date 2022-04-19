#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

#include "Param.h"
#include "State.h"

namespace audio
{
	using MIDIBuf = juce::MidiBuffer;

	class MIDILearn
	{
		using Param = param::Param;
		using Params = param::Params;
		using State = sta::State;
		using String = juce::String;

		static constexpr float ValInv = 1.f / 128.f;

		struct CC
		{
			CC() :
				param(nullptr)
			{}

			void setValue(int _value)
			{
				auto p = param.load();
				if (p == nullptr)
					return;

				const auto value = static_cast<float>(_value) * ValInv;

				p->setValueWithGesture(value);
			}

			std::atomic<Param*> param;
		};

	public:
		MIDILearn(Params& _params, State& _state) :
			ccBuf(),
			assignableParam(nullptr),
			params(_params),
			state(_state)
		{
		}

		void savePatch() const
		{
			for (auto i = 0; i < ccBuf.size(); ++i)
			{
				const auto& cc = ccBuf[i];
				const auto prm = cc.param.load();
				if (prm != nullptr)
					state.set(getIDString(i), "id", param::toID(param::toString(prm->id)), true);
			}
		}
		void loadPatch()
		{
			for (auto i = 0; i < ccBuf.size(); ++i)
			{
				auto var = state.get(getIDString(i), "id");
				if (var)
				{
					auto idStr = var->toString();

					auto& cc = ccBuf[i];
					const auto pID = param::toPID(idStr);
					cc.param.store(params[pID]);
				}
			}
		}

		void operator()(const MIDIBuf& midiBuf) noexcept
		{
			for (auto midi : midiBuf)
			{
				auto msg = midi.getMessage();

				if (msg.isController())
				{
					const auto ccIdx = msg.getControllerNumber();
					auto& cc = ccBuf[ccIdx];

					auto ap = assignableParam.load();
					if (ap != nullptr)
					{
						cc.param.store(ap);
						assignableParam.store(nullptr);
					}

					cc.setValue(msg.getControllerValue());
				}
			}
		}

		void assignParam(Param* param) noexcept
		{
			assignableParam.store(param);
		}
		void removeParam(Param* param) noexcept
		{
			for (auto& cc : ccBuf)
				if (param == cc.param)
					cc.param.store(nullptr);
		}

	protected:
		std::array<CC, 128> ccBuf;
		std::atomic<Param*> assignableParam;
		Params& params;
		State& state;

		String getIDString(int idx) const
		{
			return "midilearn/cc" + String(idx);
		}
	};
}