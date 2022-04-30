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
		Power,

		// low level parameters
		CrushGain,
		AnotherDummyParam,

		NumParams
	};
	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	static constexpr int MinLowLevelIdx = static_cast<int>(PID::Power) + 1;
	static constexpr int NumLowLevelParams = NumParams - MinLowLevelIdx;

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
		case PID::StereoConfig: return "Stereo Config";

		case PID::Power: return "Power";

		// LOW LEVEL PARAMS:
		case PID::CrushGain: return "Crush Gain";
		case PID::AnotherDummyParam: return "AnotherDummyParam";
		
		default: return "Invalid Parameter Name";
		}
	}

	inline PID toPID(const String& id)
	{
		for (auto i = 0; i < NumParams; ++i)
		{
			auto pID = static_cast<PID>(i);
			if (id == toID(toString(pID)))
				return pID;
		}
		return PID::NumParams;
	}

	inline String toTooltip(PID pID)
	{
		switch (pID)
		{
		case PID::Macro: return "Dial in the desired amount of macro modulation depth.";
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

		case PID::CrushGain: return "Default Parameter Tooltip Txt";

		default: return "Invalid Tooltip.";
		}
	}

	enum class Unit
	{
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

	class Param :
		public ParameterBase
	{
		static constexpr float BiasEps = .000001f;
	public:
		Param(const PID pID, const Range& _range, const float _valDenormDefault,
			const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal,
			State& _state, const Unit _unit = Unit::NumUnits, bool _locked = false) :

			AudioProcessorParameter(),
			id(pID),
			range(_range),

			state(_state),
			valDenormDefault(_valDenormDefault),
			
			valNorm(range.convertTo0to1(_valDenormDefault)),
			maxModDepth(0.f),
			valMod(valNorm.load()),
			modBias(.5f),

			valToStr(_valToStr),
			strToVal(_strToVal),
			unit(_unit),

			locked(_locked),
			inGesture(false)
		{
		}

		void savePatch(juce::ApplicationProperties& appProps) const
		{
			const auto v = range.convertFrom0to1(getValue());
			state.set(getIDString(), "value", v, true);
			const auto mdd = getMaxModDepth();
			state.set(getIDString(), "maxmoddepth", mdd, true);
			const auto mb = getModBias();
			state.set(getIDString(), "modbias", mb, true);
			
			auto user = appProps.getUserSettings();
			if (user->isValidFile())
			{
				user->setValue(getIDString() + "valDefault", valDenormDefault);
			}
		}
		void loadPatch(juce::ApplicationProperties& appProps)
		{
			const auto lckd = isLocked();
			if (!lckd)
			{
				auto var = state.get(getIDString(), "value");
				if (var)
				{
					const auto val = static_cast<float>(*var);
					const auto valD = range.convertTo0to1(val);
					setValueNotifyingHost(valD);
				}
				var = state.get(getIDString(), "maxmoddepth");
				if (var)
				{
					const auto val = static_cast<float>(*var);
					setMaxModDepth(val);
				}
				var = state.get(getIDString(), "modbias");
				if (var)
				{
					const auto val = static_cast<float>(*var);
					setModBias(val);
				}
				auto user = appProps.getUserSettings();
				if (user->isValidFile())
				{
					const auto vdd = user->getDoubleValue(getIDString() + "valDefault", static_cast<double>(valDenormDefault));
					setDefaultValue(range.convertTo0to1(static_cast<float>(vdd)));
				}
			}
		}

		//called by host, normalized, thread-safe
		float getValue() const override { return valNorm.load(); }
		float getValueDenorm() const noexcept { return range.convertFrom0to1(getValue()); }

		// called by host, normalized, avoid locks, not used by editor
		void setValue(float normalized) override
		{
			if(!isLocked())
				valNorm.store(normalized);
		}
		
		// called by editor
		bool isInGesture() const noexcept
		{
			return inGesture.load();
		}
		void setValueWithGesture(float norm)
		{
			if (isInGesture())
				return;
			beginChangeGesture();
			setValueNotifyingHost(norm);
			endChangeGesture();
		}
		void beginGesture()
		{
			inGesture.store(true);
			beginChangeGesture();
		}
		void endGesture()
		{
			inGesture.store(false);
			endChangeGesture();
		}
		float getMaxModDepth() const noexcept { return maxModDepth.load(); }
		void setMaxModDepth(float v) noexcept
		{
			if (isLocked())
				return;

			maxModDepth.store(juce::jlimit(-1.f, 1.f, v));
		}
		float getValMod() const noexcept { return valMod.load(); }
		float getValModDenorm() const noexcept { return range.convertFrom0to1(valMod.load()); }
		void setModBias(float b) noexcept
		{
			if (isLocked())
				return;
			
			b = juce::jlimit(BiasEps, 1.f - BiasEps, b);
			modBias.store(b);
		}
		float getModBias() const noexcept { return modBias.load(); }
		void setDefaultValue(float norm) noexcept
		{
			valDenormDefault = range.convertFrom0to1(norm);
		}

		// called by processor to update modulation value(s)
		void modulate(float macro) noexcept
		{
			const auto norm = getValue();

			const auto mmd = maxModDepth.load();
			const auto pol = mmd > 0.f ? 1.f : -1.f;
			const auto md = mmd * pol;
			const auto mdSkew = biased(0.f, md, modBias.load(), macro);
			const auto mod = mdSkew * pol;
			
			valMod.store(juce::jlimit(0.f, 1.f, norm + mod));
		}

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
		//float val0, val1;
	protected:
		State& state;
		float valDenormDefault;
		std::atomic<float> valNorm, maxModDepth, valMod, modBias;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;

		std::atomic<bool> locked, inGesture;
	private:
		String getIDString() const
		{
			return "params/" + toID(toString(id));
		}

		inline float biased(float start, float end, float bias/*[0,1]*/, float x) const noexcept
		{
			const auto r = end - start;
			if (r == 0.f)
				return 0.f;
			const auto a2 = 2.f * bias;
			const auto aM = 1.f - bias;
			const auto aR = r * bias;
			return start + aR * x / (aM - x + a2 * x);
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
				else if (txt.contains("*"))
				{
					for (auto i = 0; i < txt.length(); ++i)
					{
						if (txt[i] == '*')
						{
							const auto a = txt.substring(0, i).getFloatValue();
							const auto b = txt.substring(i + 1).getFloatValue();
							return a * b;
						}
					}
				}
				return altVal;
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
			return [](const String& txt) { return std::floor(txt.trimCharactersAtEnd(toString(Unit::Octaves)).getFloatValue()); };
		}
		inline StrToValFunc oct2()
		{
			return [](const String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Octaves)).getFloatValue() / 12.f; };
		}
		inline StrToValFunc semi()
		{
			return [](const String& txt) { return std::floor(txt.trimCharactersAtEnd(toString(Unit::Semi)).getFloatValue()); };
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
		inline ValToStrFunc mute()
		{
			return [](float v) { return v > .5f ? "Mute" : "Not Mute"; };
		}
		inline ValToStrFunc solo()
		{
			return [](float v) { return v > .5f ? "Solo" : "Not Solo"; };
		}
		inline ValToStrFunc power()
		{
			return [](float v) { return v > .5f ? "Enabled" : "Disabled"; };
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
			params.push_back(makeParam(PID::Power, state, 1.f, makeRange::toggle(), Unit::Power));

			// LOW LEVEL PARAMS:
			params.push_back(makeParam(PID::CrushGain, state, 32.f, makeRange::withCentre(1.f, 128.f, 32.f)));
			params.push_back(makeParam(PID::AnotherDummyParam, state, .5f, makeRange::withCentre(0.f, 1.f, .7f)));
			
			// LOW LEVEL PARAMS END

			for (auto param : params)
				audioProcessor.addParameter(param);
		}

		void loadPatch(juce::ApplicationProperties& appProps)
		{
			for (auto param : params)
				param->loadPatch(appProps);
		}
		void savePatch(juce::ApplicationProperties& appProps) const
		{
			for (auto param : params)
				param->savePatch(appProps);
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

		Parameters& data() noexcept { return params; }
		const Parameters& data() const noexcept { return params; }
	protected:
		Parameters params;
	};

	struct MacroProcessor
	{
		MacroProcessor(Params& _params) :
			params(_params)
		{
		}

		void operator()() noexcept
		{
			const auto modDepth = params[PID::Macro]->getValue();
			for (auto i = 1; i < NumParams; ++i)
				params[i]->modulate(modDepth);
		}

		Params& params;
	};
}

#include "configEnd.h"