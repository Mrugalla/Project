#pragma once

#include <functional>

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "../arch/State.h"
#include "../arch/Range.h"
#include "../arch/Smooth.h"

namespace param
{
	using String = juce::String;
	using AudioBuffer = juce::AudioBuffer<float>;
	using Smooth = smooth::Lowpass<float>;
	using SIMD = juce::FloatVectorOperations;

	String toID(const String&);

	enum class PID
	{
		// high level params
		Macro,
#if PPDHasGainIn
		GainIn,
#endif
		Mix,
		Gain,
#if PPDHasPolarity
		Polarity,
#endif
#if PPDHasUnityGain && PPDHasGainIn
		UnityGain,
#endif
#if PPDHasHQ
		HQ,
#endif
#if PPDHasStereoConfig
		StereoConfig,
#endif
		Power,

		// low level parameters
		RingModFreq,

		NumParams
	};

	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	static constexpr int MinLowLevelIdx = static_cast<int>(PID::Power) + 1;
	static constexpr int NumLowLevelParams = NumParams - MinLowLevelIdx;

	PID ll(PID, int/*offset*/) noexcept;

	String toString(PID);

	PID toPID(const String&);

	String toTooltip(PID);

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
		Voices,
		Pan,
		FilterType,
		NumUnits
	};

	using CharPtr = juce::CharPointer_UTF8;

	String toString(Unit);

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
			State&, const Unit = Unit::NumUnits,
			bool = false, bool = false, bool = false);

		/*blockSize*/
		void prepare(int);
		
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

		float** getValModNorm() noexcept;

		const float** getValModNorm() const noexcept;

		float** getValModDenorm() noexcept;

		const float** getValModDenorm() const noexcept;

		void setDefaultValue(float/*norm*/) noexcept;

		// called by processor to update modulation value(s)
		void processBlockInit(int, int) noexcept;
		
		void modulate(float**, int, int) noexcept;

		void processBlockEnd(int, int) noexcept;

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

		static String getIDString(PID);

		const PID id;
		const Range range;
		const int numChannelsMax;
		int blockSize;
	protected:
		State& state;
		float valDenormDefault;
		std::atomic<float> value;
		AudioBuffer valModNorm, valModDenorm;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;

		std::atomic<bool> locked, inGesture;

		const bool needsDenorm;
	};

	struct Params
	{
		using AudioProcessor = juce::AudioProcessor;
		using Parameters = std::vector<Param*>;

		Params(AudioProcessor&, State&);

		/* blockSize */
		void prepare(int);
		
		void loadPatch(juce::ApplicationProperties&);

		void savePatch(juce::ApplicationProperties&) const;

		static String getIDString();

		int getParamIdx(const String& /*nameOrID*/) const;

		size_t numParams() const noexcept;

		bool isModDepthLocked() const noexcept;
		void setModDepthLocked(bool) noexcept;
		void switchModDepthLocked() noexcept;

		void processBlockInit(int, int) noexcept;

		void processBlockEnd(int, int) noexcept;
		
		Param* operator[](int) noexcept;
		const Param* operator[](int) const noexcept;
		Param* operator[](PID) noexcept;
		const Param* operator[](PID) const noexcept;

		Parameters& data() noexcept;
		const Parameters& data() const noexcept;
	protected:
		Parameters params;

		State& state;
		std::atomic<float> modDepthLocked;
	};

	namespace strToVal
	{
		std::function<float(String, const float/*altVal*/)> parse();

		StrToValFunc power();
		StrToValFunc solo();
		StrToValFunc mute();
		StrToValFunc percent();
		StrToValFunc hz();
		StrToValFunc phase();
		StrToValFunc oct();
		StrToValFunc oct2();
		StrToValFunc semi();
		StrToValFunc fine();
		StrToValFunc ratio();
		StrToValFunc lrms();
		StrToValFunc freeSync();
		StrToValFunc polarity();
		StrToValFunc ms();
		StrToValFunc db();
		StrToValFunc voices();
		StrToValFunc pan(Params&);
		StrToValFunc filterType();
	}

	namespace valToStr
	{
		ValToStrFunc mute();
		ValToStrFunc solo();
		ValToStrFunc power();
		ValToStrFunc percent();
		ValToStrFunc hz();
		ValToStrFunc phase();
		ValToStrFunc phase360();
		ValToStrFunc oct();
		ValToStrFunc oct2();
		ValToStrFunc semi();
		ValToStrFunc fine();
		ValToStrFunc ratio();
		ValToStrFunc lrms();
		ValToStrFunc freeSync();
		ValToStrFunc polarity();
		ValToStrFunc ms();
		ValToStrFunc db();
		ValToStrFunc empty();
		ValToStrFunc voices();
		ValToStrFunc pan(Params&);
		ValToStrFunc filterType();
	}

	/* PID, State, valDenormDefault, Range, Unit, isBuffered, isStereo, needsDenorm */
	Param* makeParam(PID, State&,
		float = 1.f, const Range& = Range(0.f, 1.f),
		Unit = Unit::Percent, bool = false, bool = false, bool = false);

	Param* makeParamPan(PID, State&, Params&);

	enum class ModType
	{
		Macro,
		EnvFol,
		EnvGen1,
		EnvGen2,
		LFO1,
		LFO2,
		Randomizer1,
		Randomizer2,
		NumMods
	};
	
	static constexpr int NumMods = static_cast<int>(ModType::NumMods);
	String toString(ModType);
	
	ModType fromString(const String&);
	
	struct Mod
	{
		Mod(ModType, std::vector<Param*> && = {});
		
		bool operator==(ModType) const noexcept;
		
		bool hasParam(Param*) const noexcept;

		bool hasParam(Param&) const noexcept;

		/* sampleRate, blockSize */
		void prepare(float, int);
		
		/* numChannels, numSamples */
		void processBlock(int, int) noexcept;

		const float** data() const noexcept;
		
		const ModType type;
	protected:
		const std::vector<Param*> params;
		std::array<Smooth, 2> smooths;
		AudioBuffer buffer;

		/* modBuffer, numChannels, numSamples */
		void processBlockMacro(float**, int, int) noexcept;
	};

	using Mods = std::array<Mod, NumMods>;
	
	struct Dest
	{
		static constexpr int RemapCurveSize = 1 << 11;
		static constexpr float RemapCurveSizeF = static_cast<float>(RemapCurveSize);
		
		Dest(Params&, const Mods&);

		/* blockSize */
		void prepare(int);

		/* numChannels, numSamples */
		void operator()(int, int) noexcept;
		
		Params& params;
		const Mods& mods;
		AudioBuffer destBuffer;
		std::array<float, RemapCurveSize + 4> remapCurve;
		float modDepth, bidirectional;
		int pIdx, mIdx;
		bool active;
	};

	String toString(Dest&);

	static constexpr int NumDests = 8;
	using DestArray = std::array<Dest, NumDests>;
	
	constexpr auto sequenceForDestArray(const std::array<Dest, NumDests>&)
	{
		return std::make_index_sequence<NumDests>();
	}

	struct Dests
	{
		Dests(Params&, Mods&);

		void prepare(int);

		void operator()(int, int) noexcept;

		DestArray dests;
		
	private:
		template <std::size_t... Ix>
		Dests(Params& p, Mods& m, std::index_sequence<Ix...>)
			: dests{ {(Ix, Dest{p, m})...} }
		{}
	};
	
	String toString(Dests&);
	
	struct ModSys
	{
		ModSys(State&, Params&);

		/* sampleRate, blockSize */
		void prepare(float, int);
		
		/* numChannels, numSamples */
		void operator()(int, int) noexcept;
		
	protected:
		State& state;
		Params& params;
		Mods mods;
		std::array<int, NumMods> modsIdx;
		Dests dests;
	};
		
}