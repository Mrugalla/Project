#include "Param.h"
#include "../arch/FormularParser.h"
#include "../arch/Conversion.h"

namespace param
{
	String toID(const String& name)
	{
		return name.removeCharacters(" ").toLowerCase();
	}

	PID ll(PID pID, int offset) noexcept
	{
		auto i = static_cast<int>(pID);
		i += (NumLowLevelParams - 1) * offset;
		return static_cast<PID>(i);
	}

	String toString(PID pID)
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
#if PPDHasPolarity
		case PID::Polarity: return "Polarity";
#endif
#if PPDHasUnityGain && PPDHasGainIn
		case PID::UnityGain: return "Unity Gain";
#endif
#if PPDHasStereoConfig
		case PID::StereoConfig: return "Stereo Config";
#endif

		case PID::Power: return "Power";

			// LOW LEVEL PARAMS:
		case PID::RingModFreq: return "Ring Mod Freq";

		default: return "Invalid Parameter Name";
		}
	}

	PID toPID(const String& id)
	{
		const auto nID = toID(id);
		for (auto i = 0; i < NumParams; ++i)
		{
			auto pID = static_cast<PID>(i);
			if (nID == toID(toString(pID)))
				return pID;
		}
		return PID::NumParams;
	}

	String toTooltip(PID pID)
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
#if PPDHasPolarity
		case PID::Polarity: return "Invert the wet signal's polarity.";
#endif
#if PPDHasUnityGain && PPDHasGainIn
		case PID::UnityGain: return "If enabled the inversed input gain gets added to the output gain.";
#endif
#if PPDHasStereoConfig
		case PID::StereoConfig: return "Define the stereo-configuration. L/R or M/S.";
#endif

		case PID::Power: return "Bypass the plugin with this parameter.";

		case PID::RingModFreq: return "RingModFreq";

		default: return "Invalid Tooltip.";
		}
	}

	String toString(Unit pID)
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
		case Unit::Voices: return "v";
		case Unit::Pan: return "%";
		case Unit::FilterType: return "";
		default: return "";
		}
	}

	// PARAM:

	Param::Param(const PID pID, const Range& _range, const float _valDenormDefault,
		const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal,
		State& _state, const Unit _unit, bool _isBuffered, bool _isStereo, bool _needsDenorm) :

		AudioProcessorParameter(),
		id(pID),
		range(_range),
		numChannelsMax(_isStereo ? 2 : 1),
		blockSize(_isBuffered ? 1 : 0),

		state(_state),
		valDenormDefault(_valDenormDefault),

		value(range.convertTo0to1(_valDenormDefault)),
		valModNorm(),
		valModDenorm(),

		valToStr(_valToStr),
		strToVal(_strToVal),
		unit(_unit),

		locked(false),
		inGesture(false),

		needsDenorm(_needsDenorm)
	{
	}

	void Param::prepare(int _blockSize)
	{
		blockSize = _blockSize == 0 ? 1 : _blockSize;

		valModNorm.setSize(numChannelsMax, blockSize, false, false, false);
		if(needsDenorm)
			valModDenorm.setSize(numChannelsMax, blockSize, false, false, false);
	}

	void Param::savePatch(juce::ApplicationProperties& appProps) const
	{
		const auto idStr = getIDString(id);

		const auto v = range.convertFrom0to1(getValue());
		state.set(idStr, "value", v, true);
		//const auto mdd = getMaxModDepth();
		//state.set(idStr, "maxmoddepth", mdd, true);
		//const auto mb = getModBias();
		//state.set(idStr, "modbias", mb, true);

		auto user = appProps.getUserSettings();
		if (user->isValidFile())
		{
			user->setValue(idStr + "valDefault", valDenormDefault);
		}
	}

	void Param::loadPatch(juce::ApplicationProperties& appProps)
	{
		const auto idStr = getIDString(id);

		const auto lckd = isLocked();
		if (!lckd)
		{
			auto var = state.get(idStr, "value");
			if (var)
			{
				const auto val = static_cast<float>(*var);
				const auto valD = range.convertTo0to1(range.snapToLegalValue(val));
				setValueNotifyingHost(valD);
			}
			var = state.get(idStr, "maxmoddepth");
			if (var)
			{
				//const auto val = static_cast<float>(*var);
				//setMaxModDepth(val);
			}
			var = state.get(idStr, "modbias");
			if (var)
			{
				//const auto val = static_cast<float>(*var);
				//setModBias(val);
			}

			auto user = appProps.getUserSettings();
			if (user->isValidFile())
			{
				const auto vdd = user->getDoubleValue(idStr + "valDefault", static_cast<double>(valDenormDefault));
				setDefaultValue(range.convertTo0to1(range.snapToLegalValue(static_cast<float>(vdd))));
			}
		}
	}

	//called by host, normalized, thread-safe
	float Param::getValue() const
	{
		return value.load();
	}
	
	float Param::getValueDenorm() const noexcept
	{
		return range.convertFrom0to1(getValue());
	}

	// called by host, normalized, avoid locks, not used (directly) by editor
	void Param::setValue(float normalized)
	{
		if (isLocked())
			return;

		//if (!modDepthLocked)
			return value.store(normalized);

		//const auto p0 = valNorm.load();
		//const auto p1 = normalized;

		//const auto d0 = getMaxModDepth();
		//const auto d1 = d0 - p1 + p0;

		//valNorm.store(p1);
		//setMaxModDepth(d1);
	}

	// called by editor
	bool Param::isInGesture() const noexcept
	{
		return inGesture.load();
	}

	void Param::setValueWithGesture(float norm)
	{
		if (isInGesture())
			return;
		beginChangeGesture();
		setValueNotifyingHost(norm);
		endChangeGesture();
	}

	void Param::beginGesture()
	{
		inGesture.store(true);
		beginChangeGesture();
	}

	void Param::endGesture()
	{
		inGesture.store(false);
		endChangeGesture();
	}

	float** Param::getValModNorm() noexcept
	{
		return valModNorm.getArrayOfWritePointers();
	}

	const float** Param::getValModNorm() const noexcept
	{
		return valModNorm.getArrayOfReadPointers();
	}

	float** Param::getValModDenorm() noexcept
	{
		return valModDenorm.getArrayOfWritePointers();
	}

	const float** Param::getValModDenorm() const noexcept
	{
		return valModDenorm.getArrayOfReadPointers();
	}
	
	void Param::setDefaultValue(float norm) noexcept
	{
		valDenormDefault = range.convertFrom0to1(norm);
	}

	// called by modSys to initialize parameter updates
	void Param::processBlockInit(int numChannels, int numSamples) noexcept
	{
		auto valNorm = getValue();

		numChannels = std::min(numChannels, numChannelsMax);
		numSamples = std::min(numSamples, blockSize);
		
		auto vmn = valModNorm.getArrayOfWritePointers();
		
		for (auto ch = 0; ch < numChannels; ++ch)
			SIMD::fill(vmn[ch], valNorm, numSamples);
	}
	
	// called by modSys to add modulation input to parameter buffer
	void Param::modulate(float** modBuffer, int numChannels, int numSamples) noexcept
	{
		auto vmn = valModNorm.getArrayOfWritePointers();
		
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto vm = vmn[ch];
			const auto mb = modBuffer[ch];

			SIMD::add(vm, mb, numSamples);
		}
	}

	void Param::processBlockEnd(int numChannels, int numSamples) noexcept
	{
		if (!needsDenorm)
			return;
		
		numChannels = std::min(numChannels, numChannelsMax);
		numSamples = std::min(numSamples, blockSize);

		const auto vmn = getValModNorm();
		auto vmd = valModDenorm.getArrayOfWritePointers();

		for (auto ch = 0; ch < numChannels; ++ch)
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto smplNorm = vmn[ch][s];
				const auto smplDenorm = range.convertFrom0to1(smplNorm);
				vmd[ch][s] = smplDenorm;
			}
				
	}

	float Param::getDefaultValue() const
	{
		return range.convertTo0to1(valDenormDefault);
	}

	String Param::getName(int) const
	{
		return toString(id);
	}

	// units of param (hz, % etc.)
	String Param::getLabel() const
	{
		return toString(unit);
	}

	// string of norm val
	String Param::getText(float norm, int) const
	{
		return valToStr(range.snapToLegalValue(range.convertFrom0to1(norm)));
	}

	// string to norm val
	float Param::getValueForText(const String& text) const
	{
		const auto val = juce::jlimit(range.start, range.end, strToVal(text));
		return range.convertTo0to1(val);
	}

	// string to denorm val
	float Param::getValForTextDenorm(const String& text) const
	{
		return strToVal(text);
	}

	String Param::_toString()
	{
		auto v = getValue();
		return getName(10) + ": " + String(v) + "; " + getText(v, 10);
	}

	bool Param::isLocked() const noexcept
	{
		return locked.load();
	}

	void Param::setLocked(bool e) noexcept
	{
		locked.store(e);
	}

	void Param::switchLock() noexcept
	{
		setLocked(!isLocked());
	}

	String Param::getIDString(PID pID)
	{
		return "params/" + toID(toString(pID));
	}
}

namespace param::strToVal
{
	std::function<float(String, const float/*altVal*/)> parse()
	{
		return [](const String& txt, const float altVal)
		{
			parser::Parser parse;
			if (parse(txt))
				return parse[0];

			return altVal;
		};
	}

	StrToValFunc power()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Power));
			const auto val = p(text, 0.f);
			return val > .5f ? 1.f : 0.f;
		};
	}

	StrToValFunc solo()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Solo));
			const auto val = p(text, 0.f);
			return val > .5f ? 1.f : 0.f;
		};
	}

	StrToValFunc mute()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Mute));
			const auto val = p(text, 0.f);
			return val > .5f ? 1.f : 0.f;
		};
	}

	StrToValFunc percent()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Percent));
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc hz()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Hz)).toLowerCase();
			
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc phase()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Degree));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc oct()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Octaves));
			const auto val = p(text, 0.f);
			return std::floor(val);
		};
	}

	StrToValFunc oct2()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Octaves));
			const auto val = p(text, 0.f);
			return val / 12.f;
		};
	}

	StrToValFunc semi()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Semi));
			const auto val = p(text, 0.f);
			return std::floor(val);
		};
	}

	StrToValFunc fine()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Fine));
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc ratio()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Ratio));
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc lrms()
	{
		return [](const String& txt)
		{
			return txt[0] == 'l' ? 0.f : 1.f;
		};
	}

	StrToValFunc freeSync()
	{
		return [](const String& txt)
		{
			return txt[0] == 'f' ? 0.f : 1.f;
		};
	}

	StrToValFunc polarity()
	{
		return [](const String& txt)
		{
			return txt[0] == '0' ? 0.f : 1.f;
		};
	}

	StrToValFunc ms()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Ms));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc db()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Decibel));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc voices()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Voices));
			const auto val = p(text, 1.f);
			return val;
		};
	}

	StrToValFunc pan(Params& params)
	{
		return[p = parse(), &prms = params](const String& txt)
		{
			if (txt == "center" || txt == "centre")
				return 0.f;

			const auto text = txt.trimCharactersAtEnd("MSLR").toLowerCase();
#if PPDHasStereoConfig
			const auto sc = prms[PID::StereoConfig];
			auto isMidSide = sc->getValModNorm()[0][0];
			if (isMidSide < .5f)
#endif
			{
				if (txt == "l" || txt == "left")
					return -1.f;
				else if (txt == "r" || txt == "right")
					return 1.f;
			}
#if PPDHasStereoConfig
			else
			{

				if (txt == "m" || txt == "mid")
					return -1.f;
				else if (txt == "s" || txt == "side")
					return 1.f;
			}
#endif
				
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc filterType()
	{
		return[p = parse()](const String& txt)
		{
			/*
			Lowpass,
			Highpass,
			Bandpass,
			Bell,
			Notch,
			Allpass
			*/
			if (txt == "lowpass" || txt == "lp")
				return 0.f;
			else if (txt == "highpass" || txt == "hp")
				return 1.f;
			else if (txt == "bandpass" || txt == "bp")
				return 2.f;
			else if (txt == "bell" || txt == "b")
				return 3.f;
			else if (txt == "notch" || txt == "n")
				return 4.f;
			else if (txt == "allpass" || txt == "ap")
				return 5.f;
			else
				return p(txt, 0.f);

			//const auto text = txt.trimCharactersAtEnd("MSLR").toLowerCase();
		};
	}


}

namespace param::valToStr
{
	ValToStrFunc mute()
	{
		return [](float v) { return v > .5f ? "Mute" : "Not Mute"; };
	}

	ValToStrFunc solo()
	{
		return [](float v) { return v > .5f ? "Solo" : "Not Solo"; };
	}

	ValToStrFunc power()
	{
		return [](float v) { return v > .5f ? "Enabled" : "Disabled"; };
	}

	ValToStrFunc percent()
	{
		return [](float v) { return String(std::floor(v * 100.f)) + " " + toString(Unit::Percent); };
	}

	ValToStrFunc hz()
	{
		return [](float v)
		{
			if (v >= 10000.f)
				return String(v).substring(0, 5) + " " + toString(Unit::Hz);
			else if (v >= 1000.f)
				return String(v).substring(0, 4) + " " + toString(Unit::Hz);
			else
				return String(v).substring(0, 5) + " " + toString(Unit::Hz);
		};
	}

	ValToStrFunc phase()
	{
		return [](float v) { return String(std::floor(v * 180.f)) + " " + toString(Unit::Degree); };
	}

	ValToStrFunc phase360()
	{
		return [](float v) { return String(std::floor(v * 360.f)) + " " + toString(Unit::Degree); };
	}

	ValToStrFunc oct()
	{
		return [](float v) { return String(std::floor(v)) + " " + toString(Unit::Octaves); };
	}

	ValToStrFunc oct2()
	{
		return [](float v) { return String(std::floor(v / 12.f)) + " " + toString(Unit::Octaves); };
	}

	ValToStrFunc semi()
	{
		return [](float v) { return String(std::floor(v)) + " " + toString(Unit::Semi); };
	}

	ValToStrFunc fine()
	{
		return [](float v) { return String(std::floor(v * 100.f)) + " " + toString(Unit::Fine); };
	}

	ValToStrFunc ratio()
	{
		return [](float v)
		{
			const auto y = static_cast<int>(std::floor(v * 100.f));
			return String(100 - y) + " : " + String(y);
		};
	}

	ValToStrFunc lrms()
	{
		return [](float v) { return v > .5f ? String("m/s") : String("l/r"); };
	}

	ValToStrFunc freeSync()
	{
		return [](float v) { return v > .5f ? String("sync") : String("free"); };
	}

	ValToStrFunc polarity()
	{
		return [](float v) { return v > .5f ? String("on") : String("off"); };
	}

	ValToStrFunc ms()
	{
		return [](float v) { return String(std::floor(v * 10.f) * .1f) + " " + toString(Unit::Ms); };
	}

	ValToStrFunc db()
	{
		return [](float v) { return String(std::floor(v * 100.f) * .01f) + " " + toString(Unit::Decibel); };
	}

	ValToStrFunc empty()
	{
		return [](float) { return String(""); };
	}

	ValToStrFunc voices()
	{
		return [](float v)
		{
			return String(static_cast<int>(v)) + toString(Unit::Voices);
		};
	}

	ValToStrFunc pan(Params& params)
	{
		return [&prms = params](float v)
		{
			if (v == 0.f)
				return String("C");

#if PPDHasStereoConfig
			const auto sc = prms[PID::StereoConfig];
			const auto vm = sc->getValModNorm()[0][0];
			const auto isMidSide = vm > .5f;

			if (!isMidSide)
#endif
			{
				if (v == -1.f)
					return String("Left");
				else if (v == 1.f)
					return String("Right");
				else
					return String(std::floor(v * 100.f)) + (v < 0.f ? " L" : " R");
			}
#if PPDHasStereoConfig
			else
			{
				if (v == -1.f)
					return String("Mid");
				else if (v == 1.f)
					return String("Side");
				else
					return String(std::floor(v * 100.f)) + (v < 0.f ? " M" : " S");
			}
#endif
		};
	}

	ValToStrFunc filterType()
	{
		return [](float v)
		{
			/*
			Lowpass,
			Highpass,
			Bandpass,
			Bell,
			Notch,
			Allpass
			*/
			if (v < .5f)
				return String("LP");
			else if (v < 1.5f)
				return String("HP");
			else if (v < 2.5f)
				return String("BP");
			else if (v < 3.5f)
				return String("Bell");
			else if (v < 4.5f)
				return String("Notch");
			else
				return String("AP");
		};
	}

}

namespace param
{
	Param* makeParam(PID id, State& state,
		float valDenormDefault, const Range& range,
		Unit unit, bool isBuffered, bool isStereo, bool needsDenorm)
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
		case Unit::Semi:
			valToStrFunc = valToStr::semi();
			strToValFunc = strToVal::semi();
			break;
		case Unit::Fine:
			valToStrFunc = valToStr::fine();
			strToValFunc = strToVal::fine();
			break;
		case Unit::Voices:
			valToStrFunc = valToStr::voices();
			strToValFunc = strToVal::voices();
			break;
		case Unit::FilterType:
			valToStrFunc = valToStr::filterType();
			strToValFunc = strToVal::filterType();
			break;
		default:
			valToStrFunc = valToStr::empty();
			strToValFunc = strToVal::percent();
			break;
		}

		return new Param(id, range, valDenormDefault, valToStrFunc, strToValFunc, state, unit,
			isBuffered, isStereo, needsDenorm);
	}

	Param* makeParamPan(PID id, State& state, Params& params)
	{
		ValToStrFunc valToStrFunc = valToStr::pan(params);
		StrToValFunc strToValFunc = strToVal::pan(params);

		return new Param(id, { -1.f, 1.f }, 0.f, valToStrFunc, strToValFunc, state,
			Unit::Pan, true, false, true);
	}

	// PARAMS

	Params::Params(AudioProcessor& audioProcessor, State& _state) :
		params(),
		state(_state),
		modDepthLocked(false)
	{
		params.push_back(makeParam(PID::Macro, state, 0.f, { 0.f, 1.f }, Unit::Percent, true, true, false));
#if PPDHasGainIn
		params.push_back(makeParam(PID::GainIn, state, 0.f, makeRange::withCentre(PPD_GainIn_Min, PPD_GainIn_Max, 0.f), Unit::Decibel, true, true, true));
#endif
		params.push_back(makeParam(PID::Mix, state, 1.f, Range(), Unit::Percent, true, true, false));
		params.push_back(makeParam(PID::Gain, state, 0.f, makeRange::withCentre(PPD_GainOut_Min, PPD_GainOut_Max, 0.f), Unit::Decibel, true, true, true));
#if PPDHasPolarity
		params.push_back(makeParam(PID::Polarity, state, 0.f, makeRange::toggle(), Unit::Polarity));
#endif
#if PPDHasUnityGain && PPDHasGainIn
		params.push_back(makeParam(PID::UnityGain, state, (PPD_UnityGainDefault ? 1.f : 0.f), makeRange::toggle(), Unit::Polarity));
#endif
#if PPDHasHQ
		params.push_back(makeParam(PID::HQ, state, 0.f, makeRange::toggle()));
#endif
#if PPDHasStereoConfig
		params.push_back(makeParam(PID::StereoConfig, state, 1.f, makeRange::toggle(), Unit::StereoConfig));
#endif
		params.push_back(makeParam(PID::Power, state, 1.f, makeRange::toggle(), Unit::Power));

		// LOW LEVEL PARAMS:
		params.push_back(makeParam(PID::RingModFreq, state, 420.f, makeRange::withCentre(0.f, 20000.f, 420.f), Unit::Hz, true, true, true));
		
		
		// LOW LEVEL PARAMS END:
		for (auto param : params)
			audioProcessor.addParameter(param);
	}

	void Params::prepare(int blockSize)
	{
		for (auto& param : params)
			param->prepare(blockSize);
	}

	void Params::loadPatch(juce::ApplicationProperties& appProps)
	{
		const auto idStr = getIDString();
		const auto mdl = state.get(idStr, "moddepthlocked");
		if (mdl != nullptr)
			setModDepthLocked(static_cast<int>(*mdl) != 0);

		for (auto param : params)
			param->loadPatch(appProps);
	}

	void Params::savePatch(juce::ApplicationProperties& appProps) const
	{
		for (auto param : params)
			param->savePatch(appProps);

		const auto idStr = getIDString();
		state.set(idStr, "moddepthlocked", isModDepthLocked() ? 1 : 0);
	}

	String Params::getIDString()
	{
		return "params";
	}

	int Params::getParamIdx(const String& nameOrID) const
	{
		for (auto p = 0; p < params.size(); ++p)
		{
			const auto pName = toString(params[p]->id);
			if (nameOrID == pName || nameOrID == toID(pName))
				return p;
		}
		return -1;
	}

	size_t Params::numParams() const noexcept { return params.size(); }

	Param* Params::operator[](int i) noexcept { return params[i]; }
	const Param* Params::operator[](int i) const noexcept { return params[i]; }
	Param* Params::operator[](PID p) noexcept { return params[static_cast<int>(p)]; }
	const Param* Params::operator[](PID p) const noexcept { return params[static_cast<int>(p)]; }

	Params::Parameters& Params::data() noexcept { return params; }

	const Params::Parameters& Params::data() const noexcept { return params; }

	bool Params::isModDepthLocked() const noexcept
	{
		return modDepthLocked.load();
	}

	void Params::setModDepthLocked(bool e) noexcept
	{
		modDepthLocked.store(e);
		//for (auto& p : params)
		//	p->setModDepthLocked(e);
	}

	void Params::switchModDepthLocked() noexcept
	{
		setModDepthLocked(!isModDepthLocked());
	}

	// modulation and stuff

	void Params::processBlockInit(int numChannels, int numSamples) noexcept
	{
		for (auto& param : params)
			param->processBlockInit(numChannels, numSamples);
	}

	void Params::processBlockEnd(int numChannels, int numSamples) noexcept
	{
		for (auto& param : params)
			param->processBlockEnd(numChannels, numSamples);
	}
}

// modulation system:

namespace param
{
	String toString(ModType t)
	{
		switch (t)
		{
		case ModType::Macro: return "Macro";
		case ModType::LFO1: return "LFO1";
		case ModType::LFO2: return "LFO2";
		case ModType::EnvFol: return "EnvFol";
		case ModType::EnvGen1: return "EnvGen1";
		case ModType::EnvGen2: return "EnvGen2";
		case ModType::Randomizer1: return "Randomizer1";
		case ModType::Randomizer2: return "Randomizer2";
		default: return "";
		}
	}
	
	ModType fromString(const juce::String& s)
	{
		if (s == "Macro") return ModType::Macro;
		if (s == "LFO1") return ModType::LFO1;
		if (s == "LFO2") return ModType::LFO2;
		if (s == "EnvFol") return ModType::EnvFol;
		if (s == "EnvGen1") return ModType::EnvGen1;
		if (s == "EnvGen2") return ModType::EnvGen2;
		if (s == "Randomizer1") return ModType::Randomizer1;
		if (s == "Randomizer2") return ModType::Randomizer2;
		return ModType::NumMods;
	}

	// MOD

	Mod::Mod(ModType _type, std::vector<Param*>&& _params) :
		type(_type),
		params(_params),
		buffer()
	{}

	bool Mod::operator==(ModType other) const noexcept
	{
		return other == type;
	}

	bool Mod::hasParam(Param* param) const noexcept
	{
		for (auto p = 0; p < params.size(); ++p)
			if (params[p]->id == param->id)
				return true;
		return false;
	}

	bool Mod::hasParam(Param& param) const noexcept
	{
		for (auto p = 0; p < params.size(); ++p)
			if (params[p]->id == param.id)
				return true;
		return false;
	}

	void Mod::prepare(float sampleRate, int blockSize)
	{
		buffer.setSize(2, blockSize, false, false, false);
		for (auto& smooth : smooths)
			smooth.makeFromDecayInMs(10.f, sampleRate);
	}

	void Mod::processBlock(int numChannels, int numSamples) noexcept
	{
		auto modBuffer = buffer.getArrayOfWritePointers();

		switch (type)
		{
		case ModType::Macro: return processBlockMacro(modBuffer, numChannels, numSamples);
		}
	}

	const float** Mod::data() const noexcept
	{
		return buffer.getArrayOfReadPointers();
	}

	void Mod::processBlockMacro(float** modBuffer, int numChannels, int numSamples) noexcept
	{
		const auto& macroParam = *params[0];
		const auto macroP = macroParam.getValModNorm();
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			const auto macro = macroP[ch];
			auto& smooth = smooths[ch];
			auto modBuf = modBuffer[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto mcr = macro[s];
				modBuf[s] = smooth(mcr);
			}
		}
	}

	// DEST

	Dest::Dest(Params& _params, const Mods& _mods) :
		params(_params),
		mods(_mods),
		remapCurve(),
		modDepth(0.f),
		bidirectional(0.f),
		pIdx(0),
		mIdx(0),
		active(false)
	{
		auto x = 0.f;
		auto inc = 1.f / static_cast<float>(remapCurve.size());

		for (auto i = 0; i < RemapCurveSize; ++i, x += inc)
			remapCurve[i] = x;

		for (auto i = 0; i < 4; ++i)
			remapCurve[RemapCurveSize + i] = remapCurve[i];
	}

	void Dest::prepare(int blockSize)
	{
		destBuffer.setSize(2, blockSize);
	}

	void Dest::operator()(int numChannels, int numSamples) noexcept
	{
		if (!active)
			return;

		auto& param = *params[pIdx];
		auto& mod = mods[mIdx];

		numChannels = std::min(numChannels, param.numChannelsMax);
		numSamples = std::min(numSamples, param.blockSize);

		const auto modBuffer = mod.data();
		auto destBuf = destBuffer.getArrayOfWritePointers();

		const auto m = modDepth;
		const auto b = bidirectional;
		const auto bm = modDepth * b;

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			const auto modBuf = modBuffer[ch];
			auto dest = destBuf[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto modSmpl = modBuf[s];
				const auto idx = static_cast<int>(modSmpl * RemapCurveSizeF);
				const auto x = remapCurve[idx];

				const auto z = x * (m + bm) - b;

				dest[s] = z;
			}
		}

		param.modulate(destBuf, numChannels, numSamples);
	}
	
	String toString(Dest& dest)
	{
		String s;
		s << toString(dest.params[dest.pIdx]->id);
		s << "; " << toString(dest.mods[dest.mIdx].type);
		s << "; " << dest.modDepth << "; " << dest.bidirectional;
		return s;
	}

	// DESTS
	
	Dests::Dests(Params& params, Mods& mods) :
		Dests(params, mods, sequenceForDestArray(dests))
	{
	}

	void Dests::prepare(int blockSize)
	{
		for (auto& dest : dests)
			dest.prepare(blockSize);
	}

	void Dests::operator()(int numChannels, int numSamples) noexcept
	{
		for (auto& dest : dests)
			dest(numChannels, numSamples);
	}

	String toString(Dests& dests)
	{
		String s;
		for (auto& dest : dests.dests)
			s << toString(dest) << "\n";
		return s;
	}

	// MODSYS

	ModSys::ModSys(State& _state, Params& _params) :
		state(_state),
		params(_params),
		mods
	{
		Mod(ModType::Macro, { params[PID::Macro]}),
		Mod(ModType::EnvFol),
		Mod(ModType::EnvGen1),
		Mod(ModType::EnvGen2),
		Mod(ModType::LFO1),
		Mod(ModType::LFO2),
		Mod(ModType::Randomizer1),
		Mod(ModType::Randomizer2)
	},
		modsIdx(),
		dests(params, mods)
	{
		for (auto i = 0; i < NumMods; ++i)
			modsIdx[i] = i;
	}

	void ModSys::prepare(float sampleRate, int blockSize)
	{
		params.prepare(blockSize);

		for (auto& mod : mods)
			mod.prepare(sampleRate, blockSize);

		dests.prepare(blockSize);
	}

	void ModSys::operator()(int numChannels, int numSamples) noexcept
	{
		// process all parameter values
		params.processBlockInit(numChannels, numSamples);

		for (auto m = 0; m < mods.size(); ++m)
		{
			const auto idx = modsIdx[m];
			auto& mod = mods[idx];

			mod.processBlock(numChannels, numSamples);
		}

		dests(numChannels, numSamples);

		params.processBlockEnd(numChannels, numSamples);
	}
}