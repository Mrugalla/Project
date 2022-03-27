#pragma once

#include <functional>
#include <array>

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "State.h"
#include "Range.h"

#include "config.h"

namespace param
{
	using String = juce::String;

	inline String toID(const String& name)
	{
		return name.removeCharacters(" ").toLowerCase();
	}

	enum class PID
	{
		// high level params
		Macro,
#if PPDHasGainIn
		GainIn,
#endif
		Mix,
#if PPDHasLookahead
		Lookahead,
#endif
		Gain,
		Polarity,
#if PPDHasUnityGain
		UnityGain,
#endif
#if PPDHasHQ
		HQ,
#endif
		StereoConfig,
		// neither high- nor lowlevel param(s)
		Power,
		PatchSelect,
		PatchMode,

		// low level parameters
		DefaultParameter,

		NumParams
	};
	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	static constexpr int MinLowLevelIdx = static_cast<int>(PID::Power) + 1;
	static constexpr int NumLowLevelParams = NumParams - MinLowLevelIdx;
	static constexpr int NumHighLevelParams = NumParams - NumLowLevelParams;
	static constexpr int MinHighLevelButton = static_cast<int>(PID::Polarity);
	static constexpr int NumHighLevelKnobs = MinHighLevelButton - 1;
#if PPDHasGainIn
	static constexpr int NumHighLevelGainKnobs = 2;
#else
	static constexpr int NumHighLevelGainKnobs = 1;
#endif

	PID ll(PID pID, int offset) noexcept
	{
		auto i = static_cast<int>(pID);
		i += (NumLowLevelParams - 1) * offset;
		return static_cast<PID>(i);
	}

	inline String toString(PID pID)
	{
		switch (pID)
		{
		case PID::Macro: return "Macro";
		case PID::PatchSelect: return "Patch Select";
		case PID::PatchMode: return "Patch Mode";
#if PPDHasGainIn
		case PID::GainIn: return "Gain In";
#endif
		case PID::Mix: return "Mix";
		case PID::Gain: return "Gain Out";
#if PPDHasHQ
		case PID::HQ: return "HQ";
#endif
#if PPDHasLookahead
		case PID::Lookahead: return "Lookahead";
#endif
		case PID::Polarity: return "Polarity";
#if PPDHasUnityGain
		case PID::UnityGain: return "Unity Gain";
#endif
		case PID::StereoConfig: return "StereoConfig";

		case PID::Power: return "Power";

		// LOW LEVEL PARAMS:
		case PID::DefaultParameter: return "Default Parameter";
		
		default: return "Invalid Parameter Name";
		}
	}

	inline String toTooltip(PID pID)
	{
		switch (pID)
		{
		case PID::Macro: return "Interpolate between the two patches X and Y.";
		case PID::PatchSelect: return "Solo one of the patches tweak it in isolation.";
		case PID::PatchMode: return "The macro can interpolate between two patch-states or two parallel-processed patches.";
#if PPDHasGainIn
		case PID::GainIn: return "Apply input gain to the wet signal.";
#endif
		case PID::Mix: return "Mix the dry with the wet signal.";
		case PID::Gain: return "Apply output gain to the wet signal.";
#if PPDHasHQ
		case PID::HQ: return "Turn on HQ to apply 2x Oversampling to the signal.";
#endif
#if PPDHasLookahead
		case PID::Lookahead: return "Dial in negative delay to compensate for this plugin's latency.";
#endif
		case PID::Polarity: return "Invert the wet signal's polarity.";
#if PPDHasUnityGain
		case PID::UnityGain: return "If enabled the inversed input gain gets added to the output gain.";
#endif
		case PID::StereoConfig: return "Define the stereo-configuration. L/R or M/S.";

		case PID::Power: return "Bypass the plugin with this parameter.";

		case PID::DefaultParameter: return "Default Parameter Tooltip Txt";

		default: return "Invalid Tooltip.";
		}
	}

	enum class Unit
	{
		PatchMode,
		PatchSelect,
		Power,
		Solo,
		Mute,
		Percent,
		Hz,
		Beats,
		Degree,
		Octaves,
		Semi,
		Fine,
		Ms,
		Decibel,
		Ratio,
		Polarity,
		StereoConfig,
		NumUnits
	};

	using CharPtr = juce::CharPointer_UTF8;

	inline String toString(Unit pID)
	{
		switch (pID)
		{
		case Unit::PatchMode: return "";
		case Unit::PatchSelect: return "";
		case Unit::Power: return "";
		case Unit::Solo: return "S";
		case Unit::Mute: return "M";
		case Unit::Percent: return "%";
		case Unit::Hz: return "hz";
		case Unit::Beats: return "x";
		case Unit::Degree: return CharPtr("\xc2\xb0");
		case Unit::Octaves: return "oct";
		case Unit::Semi: return "semi";
		case Unit::Fine: return "fine";
		case Unit::Ms: return "ms";
		case Unit::Decibel: return "db";
		case Unit::Ratio: return "ratio";
		case Unit::Polarity: return CharPtr("\xc2\xb0");
		case Unit::StereoConfig: return "";
		default: return "";
		}
	}

	using ValToStrFunc = std::function<String(float)>;
	using StrToValFunc = std::function<float(const String&)>;

	using Range = juce::NormalisableRange<float>;

	using ParameterBase = juce::AudioProcessorParameter;
	using State = sta::State;

	struct Param :
		public ParameterBase
	{
		Param(const PID pID, const Range& _range, const float _valDenormDefault,
			const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal,
			State& _state, const Unit _unit = Unit::NumUnits, bool _locked = false) :

			AudioProcessorParameter(),
			id(pID),
			range(_range),
			state(_state),
			valDenormDefault(_valDenormDefault),
			valNorm(range.convertTo0to1(_valDenormDefault)),
			valToStr(_valToStr),
			strToVal(_strToVal),
			unit(_unit),

			locked(_locked)
		{
		}

		void savePatch() const
		{
			state.set(getIDString(), "value", range.convertFrom0to1(getValue()), true);
			state.set(getIDString(), "locked", locked.load() ? 1 : 0, true);
		}
		void loadPatch()
		{
			auto var = state.get(getIDString(), "locked");
			if (var)
			{
				const auto val = static_cast<int>(*var);
				locked.store(val == 1);
			}
			var = state.get(getIDString(), "value");
			if (var)
			{
				const auto val = static_cast<float>(*var);
				const auto valD = range.convertTo0to1(val);
				setValueNotifyingHost(valD);
			}
		}

		//called by host, normalized, thread-safe
		float getValue() const override { return valNorm.load(); }
		float getValueDenorm() const noexcept { return range.convertFrom0to1(getValue()); }

		// called by host, normalized, avoid locks, not used by editor
		// use setValueNotifyingHost() from the editor
		void setValue(float normalized) override
		{
			if(!isLocked())
				valNorm.store(normalized);
		}
		void setValueWithGesture(float norm)
		{
			beginChangeGesture();
			setValueNotifyingHost(norm);
			endChangeGesture();
		}
		void beginGesture() { beginChangeGesture(); }
		void endGesture() { endChangeGesture(); }

		float getDefaultValue() const override { return range.convertTo0to1(valDenormDefault); }

		String getName(int) const override { return toString(id); }

		// units of param (hz, % etc.)
		String getLabel() const override { return toString(unit); }

		// string of norm val
		String getText(float norm, int) const override
		{
			return valToStr(range.snapToLegalValue(range.convertFrom0to1(norm)));
		}
		// string to norm val
		float getValueForText(const String& text) const override
		{
			const auto val = juce::jlimit(range.start, range.end, strToVal(text));
			return range.convertTo0to1(val);
		}
		// string to denorm val
		float getValForTextDenorm(const String& text) const { return strToVal(text); }

		String _toString()
		{
			auto v = getValue();
			return getName(10) + ": " + String(v) + "; " + getText(v, 10);
		}

		bool isLocked() const noexcept { return locked.load(); }
		void setLocked(bool e) noexcept { locked.store(e); }
		void switchLock() noexcept { setLocked(!isLocked()); }

		const PID id;
		const Range range;
	protected:
		State& state;
		const float valDenormDefault;
		std::atomic<float> valNorm;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;
		
		std::atomic<bool> locked;

	private:
		String getIDString() const
		{
			return "params/" + toID(toString(id));
		}
	};

	namespace strToVal
	{
		inline std::function<float(String, const float/*altVal*/)> division()
		{
			return [](const String& txt, const float altVal)
			{
				if (txt.contains(":") || txt.contains("/"))
				{
					for (auto i = 0; i < txt.length(); ++i)
					{
						if (txt[i] == ':' || txt[i] == '/')
						{
							const auto a = txt.substring(0, i).getFloatValue();
							const auto b = txt.substring(i + 1).getFloatValue();
							if (b != 0.f)
								return a / b;
						}
					}
				}
				return altVal;
			};
		}

		inline StrToValFunc patchMode()
		{
			return[](const String& txt)
			{
				return txt.trimCharactersAtEnd(toString(Unit::PatchMode)).getFloatValue() > .5f ? 1.f : 0.f;
			};
		}
		inline StrToValFunc patchSelect()
		{
			return[](const String& txt)
			{
				return std::rint(txt.trimCharactersAtEnd(toString(Unit::PatchSelect)).getFloatValue());
			};
		}
		inline StrToValFunc power()
		{
			return[](const String& txt)
			{
				return txt.trimCharactersAtEnd(toString(Unit::Power)).getFloatValue() > .5f ? 1.f : 0.f;
			};
		}
		inline StrToValFunc solo()
		{
			return[](const String& txt)
			{
				return txt.trimCharactersAtEnd(toString(Unit::Solo)).getFloatValue() > .5f ? 1.f : 0.f;
			};
		}
		inline StrToValFunc mute()
		{
			return[](const String& txt)
			{
				return txt.trimCharactersAtEnd(toString(Unit::Mute)).getFloatValue() > .5f ? 1.f : 0.f;
			};
		}
		inline StrToValFunc percent()
		{
			return[d = division()](const String& txt)
			{
				const auto val = d(txt, 0.f);
				if (val != 0.f)
					return val;
				return txt.trimCharactersAtEnd(toString(Unit::Percent)).getFloatValue() * .01f;
			};
		}
		inline StrToValFunc hz()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Hz)).getFloatValue(); };
		}
		inline StrToValFunc phase()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Degree)).getFloatValue(); };
		}
		inline StrToValFunc oct()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Octaves)).getIntValue(); };
		}
		inline StrToValFunc oct2()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Octaves)).getFloatValue() / 12.f; };
		}
		inline StrToValFunc semi()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Semi)).getIntValue(); };
		}
		inline StrToValFunc fine()
		{
			[](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Fine)).getFloatValue() * .01f; };
		}
		inline StrToValFunc ratio()
		{
			return[d = division()](const String& txt)
			{
				const auto val = d(txt, -1.f);
				if (val != -1.f)
					return val;
				return juce::jlimit(0.f, 1.f, txt.getFloatValue() * .01f);
			};
		}
		inline StrToValFunc lrms()
		{
			return [](const String& txt) { return txt[0] == 'l' ? 0.f : 1.f; };
		}
		inline StrToValFunc freeSync()
		{
			return [](const String& txt) { return txt[0] == 'f' ? 0.f : 1.f; };
		}
		inline StrToValFunc polarity()
		{
			return [](const String& txt) { return txt[0] == '0' ? 0.f : 1.f; };
		}
		inline StrToValFunc ms()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Ms)).getFloatValue(); };
		}
		inline StrToValFunc db()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Decibel)).getFloatValue(); };
		}
	}

	namespace valToStr
	{
		inline ValToStrFunc patchMode()
		{
			return [](float v) { return v > .5f ? "parallel-processing" : "parameters"; };
		}
		inline ValToStrFunc patchSelect()
		{
			return [](float v) { return v < .5f ? "Play All" : v < 1.5f ? "Solo A" : "Solo B"; };
		}
		inline ValToStrFunc mute()
		{
			return [](float v) { return v > .5f ? "mute" : "not mute"; };
		}
		inline ValToStrFunc solo()
		{
			return [](float v) { return v > .5f ? "solo" : "not solo"; };
		}
		inline ValToStrFunc power()
		{
			return [](float v) { return v > .5f ? "enabled" : "disabled"; };
		}
		inline ValToStrFunc percent()
		{
			return [](float v) { return String(std::floor(v * 100.f)) + " " + toString(Unit::Percent); };
		}
		inline ValToStrFunc hz()
		{
			return [](float v) { return String(v).substring(0, 4) + " " + toString(Unit::Hz); };
		}
		inline ValToStrFunc phase()
		{
			return [](float v) { return String(std::floor(v * 180.f)) + " " + toString(Unit::Degree); };
		}
		inline ValToStrFunc phase360()
		{
			return [](float v) { return String(std::floor(v * 360.f)) + " " + toString(Unit::Degree); };
		}
		inline ValToStrFunc oct()
		{
			return [](float v) { return String(std::floor(v)) + " " + toString(Unit::Octaves); };
		}
		inline ValToStrFunc oct2()
		{
			return [](float v) { return String(std::floor(v / 12.f)) + " " + toString(Unit::Octaves); };
		}
		inline ValToStrFunc semi()
		{
			return [](float v) { return String(std::floor(v)) + " " + toString(Unit::Semi); };
		}
		inline ValToStrFunc fine()
		{
			return [](float v) { return String(std::floor(v * 100.f)) + " " + toString(Unit::Fine); };
		}
		inline ValToStrFunc ratio()
		{
			return [](float v)
			{
				const auto y = static_cast<int>(std::floor(v * 100.f));
				return String(100 - y) + " : " + String(y);
			};
		}
		inline ValToStrFunc lrms()
		{
			return [](float v) { return v > .5f ? String("m/s") : String("l/r"); };
		}
		inline ValToStrFunc freeSync()
		{
			return [](float v) { return v > .5f ? String("sync") : String("free"); };
		}
		inline ValToStrFunc polarity()
		{
			return [](float v) { return v > .5f ? String("on") : String("off"); };
		}
		inline ValToStrFunc ms()
		{
			return [](float v) { return String(std::floor(v * 10.f) * .1f) + " " + toString(Unit::Ms); };
		}
		inline ValToStrFunc db()
		{
			return [](float v) { return String(std::floor(v * 100.f) * .01f) + " " + toString(Unit::Decibel); };
		}
		inline ValToStrFunc empty()
		{
			return [](float) { return String(""); };
		}
	}

	inline Param* makeParam(PID id, State& state,
		float valDenormDefault = 1.f, const Range& range = Range(0.f, 1.f),
		Unit unit = Unit::Percent, bool isLocked = false)
	{
		ValToStrFunc valToStrFunc;
		StrToValFunc strToValFunc;

		switch (unit)
		{
		case Unit::PatchSelect:
			valToStrFunc = valToStr::patchSelect();
			strToValFunc = strToVal::patchSelect();
			break;
		case Unit::Power:
			valToStrFunc = valToStr::power();
			strToValFunc = strToVal::power();
			break;
		case Unit::Solo:
			valToStrFunc = valToStr::solo();
			strToValFunc = strToVal::solo();
			break;
		case Unit::Mute:
			valToStrFunc = valToStr::mute();
			strToValFunc = strToVal::mute();
			break;
		case Unit::Decibel:
			valToStrFunc = valToStr::db();
			strToValFunc = strToVal::db();
			break;
		case Unit::Ms:
			valToStrFunc = valToStr::ms();
			strToValFunc = strToVal::ms();
			break;
		case Unit::Percent:
			valToStrFunc = valToStr::percent();
			strToValFunc = strToVal::percent();
			break;
		case Unit::Hz:
			valToStrFunc = valToStr::hz();
			strToValFunc = strToVal::hz();
			break;
		case Unit::Ratio:
			valToStrFunc = valToStr::ratio();
			strToValFunc = strToVal::ratio();
			break;
		case Unit::Polarity:
			valToStrFunc = valToStr::polarity();
			strToValFunc = strToVal::polarity();
			break;
		case Unit::StereoConfig:
			valToStrFunc = valToStr::lrms();
			strToValFunc = strToVal::lrms();
			break;
		default:
			valToStrFunc = valToStr::empty();
			strToValFunc = strToVal::percent();
			break;
		}

		return new Param(id, range, valDenormDefault, valToStrFunc, strToValFunc, state, unit, isLocked);
	}

	struct Params
	{
		using AudioProcessor = juce::AudioProcessor;
		using Parameters = std::vector<Param*>;

		Params(AudioProcessor& audioProcessor, State& state) :
			params()
		{
			params.push_back(makeParam(PID::Macro, state, 0.f));
#if PPDHasGainIn
			params.push_back(makeParam(PID::GainIn, state, 0.f, makeRange::withCentre(PPD_GainIn_Min, PPD_GainIn_Max, 0.f), Unit::Decibel));
#endif
			params.push_back(makeParam(PID::Mix, state));
#if PPDHasLookahead
			params.push_back(makeParam(PID::Lookahead, state, 0.f, makeRange::withCentre(0.f, 20.f, 4.f), Unit::Ms));
#endif
			params.push_back(makeParam(PID::Gain, state, 0.f, makeRange::withCentre(PPD_GainOut_Min, PPD_GainIn_Max, 0.f), Unit::Decibel));
			params.push_back(makeParam(PID::Polarity, state, 0.f, makeRange::toggle(), Unit::Polarity));
#if PPDHasUnityGain
			params.push_back(makeParam(PID::UnityGain, state, (PPD_UnityGainDefault ? 1.f : 0.f), makeRange::toggle(), Unit::Polarity));
#endif
#if PPDHasHQ
			params.push_back(makeParam(PID::HQ, state, 1.f, makeRange::toggle()));
#endif
			params.push_back(makeParam(PID::StereoConfig, state, 1.f, makeRange::toggle(), Unit::StereoConfig));

			params.push_back(makeParam(PID::Power, state, 0.f, makeRange::toggle(), Unit::Power));
			params.push_back(makeParam(PID::PatchSelect, state, 0.f, {0.f, 2.f}, Unit::PatchSelect));
			params.push_back(makeParam(PID::PatchMode, state, 0.f, makeRange::toggle(), Unit::PatchMode));

			// LOW LEVEL PARAMS:
			params.push_back(makeParam(PID::DefaultParameter, state));
			
			// LOW LEVEL PARAMS END

			for (auto param : params)
				audioProcessor.addParameter(param);
		}

		void loadPatch()
		{
			for (auto param : params)
				param->loadPatch();
		}
		void savePatch() const
		{
			for (auto param : params)
				param->savePatch();
		}

		int getParamIdx(const String& nameOrID) const
		{
			for (auto p = 0; p < params.size(); ++p)
			{
				const auto pName = toString(params[p]->id);
				if (nameOrID == pName || nameOrID == toID(pName))
					return p;
			}
			return -1;
		}

		size_t numParams() const noexcept { return params.size(); }

		Param* operator[](int i) noexcept { return params[i]; }
		const Param* operator[](int i) const noexcept { return params[i]; }
		Param* operator[](PID p) noexcept { return params[static_cast<int>(p)]; }
		const Param* operator[](PID p) const noexcept { return params[static_cast<int>(p)]; }
	protected:
		Parameters params;
	};

	struct MParams
	{
		struct PRM
		{
			PRM() :
				vN(0.f), vM(0.f), val(0.f),
				modDepth(0.f)
			{}

			float vN, vM, val;
			std::atomic<float> modDepth;
		};

		MParams(Params& _params) :
			params(_params),
			prms()
		{}

		void processBlock()
		{
			for (auto i = 0; i < NumParams; ++i)
			{
				const auto& param = *params[i];
				auto& prm = prms[i];

				prm.vN = param.getValue();
				prm.val = prm.vN + prm.modDepth.load() * (prm.vM - prm.vN);
			}
		}

		Params& params;
		std::array<PRM, NumParams> prms;
	};
}

#include "configEnd.h"