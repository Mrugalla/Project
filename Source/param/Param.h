#pragma once

#include "../arch/FormularParser.h"

#include <functional>
#include <array>

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "../arch/State.h"
#include "../arch/Range.h"

#include "../config.h"

namespace param
{
	using String = juce::String;

	inline String toID(const String&);

	enum class PID
	{
		// high level params
		Macro,
#if PPDHasGainIn
		GainIn,
#endif
		Mix,
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

	PID ll(PID, int/*offset*/) noexcept;

	inline String toString(PID);

	inline PID toPID(const String&);

	inline String toTooltip(PID);

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

	inline String toString(Unit);

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
		Param(const PID, const Range&, const float/*_valDenormDefault*/,
			const ValToStrFunc&, const StrToValFunc&,
			State&, const Unit = Unit::NumUnits, bool/*_locked*/ = false);

		void savePatch(juce::ApplicationProperties&) const;

		void loadPatch(juce::ApplicationProperties&);

		//called by host, normalized, thread-safe
		float getValue() const override;

		float getValueDenorm() const noexcept;

		// called by host, normalized, avoid locks, not used by editor
		void setValue(float/*normalized*/) override;
		
		// called by editor
		bool isInGesture() const noexcept;

		void setValueWithGesture(float/*norm*/);

		void beginGesture();

		void endGesture();

		float getMaxModDepth() const noexcept;

		void setMaxModDepth(float) noexcept;

		float getValMod() const noexcept;

		float getValModDenorm() const noexcept;

		void setModBias(float) noexcept;

		float getModBias() const noexcept;

		void setDefaultValue(float/*norm*/) noexcept;

		// called by processor to update modulation value(s)
		void modulate(float/*macro*/) noexcept;

		float getDefaultValue() const override;

		String getName(int) const override;

		// units of param (hz, % etc.)
		String getLabel() const override;

		// string of norm val
		String getText(float /*norm*/, int) const override;

		// string to norm val
		float getValueForText(const String&) const override;

		// string to denorm val
		float getValForTextDenorm(const String&) const;

		String _toString();

		bool isLocked() const noexcept;
		void setLocked(bool) noexcept;
		void switchLock() noexcept;

		const PID id;
		const Range range;
	protected:
		State& state;
		float valDenormDefault;
		std::atomic<float> valNorm, maxModDepth, valMod, modBias;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;

		std::atomic<bool> locked, inGesture;
	private:
		String getIDString() const;

		inline float biased(float /*start*/, float /*end*/, float /*bias [0,1]*/, float /*x*/) const noexcept;
	};

	namespace strToVal
	{
		inline std::function<float(String, const float/*altVal*/)> parse();

		inline StrToValFunc power();
		inline StrToValFunc solo();
		inline StrToValFunc mute();
		inline StrToValFunc percent();
		inline StrToValFunc hz();
		inline StrToValFunc phase();
		inline StrToValFunc oct();
		inline StrToValFunc oct2();
		inline StrToValFunc semi();
		inline StrToValFunc fine();
		inline StrToValFunc ratio();
		inline StrToValFunc lrms();
		inline StrToValFunc freeSync();
		inline StrToValFunc polarity();
		inline StrToValFunc ms();
		inline StrToValFunc db();
	}

	namespace valToStr
	{
		inline ValToStrFunc mute();
		inline ValToStrFunc solo();
		inline ValToStrFunc power();
		inline ValToStrFunc percent();
		inline ValToStrFunc hz();
		inline ValToStrFunc phase();
		inline ValToStrFunc phase360();
		inline ValToStrFunc oct();
		inline ValToStrFunc oct2();
		inline ValToStrFunc semi();
		inline ValToStrFunc fine();
		inline ValToStrFunc ratio();
		inline ValToStrFunc lrms();
		inline ValToStrFunc freeSync();
		inline ValToStrFunc polarity();
		inline ValToStrFunc ms();
		inline ValToStrFunc db();
		inline ValToStrFunc empty();
	}

	inline Param* makeParam(PID, State&,
		float /*valDenormDefault*/ = 1.f, const Range& = Range(0.f, 1.f),
		Unit = Unit::Percent, bool /*isLocked*/ = false);

	struct Params
	{
		using AudioProcessor = juce::AudioProcessor;
		using Parameters = std::vector<Param*>;

		Params(AudioProcessor&, State&);

		void loadPatch(juce::ApplicationProperties&);

		void savePatch(juce::ApplicationProperties&) const;

		int getParamIdx(const String& /*nameOrID*/) const;

		size_t numParams() const noexcept;

		Param* operator[](int) noexcept;
		const Param* operator[](int) const noexcept;
		Param* operator[](PID) noexcept;
		const Param* operator[](PID) const noexcept;

		Parameters& data() noexcept;
		const Parameters& data() const noexcept;
	protected:
		Parameters params;
	};

	struct MacroProcessor
	{
		MacroProcessor(Params&);

		void operator()() noexcept;

		Params& params;
	};
}

#include "../configEnd.h"