#include "Processor.h"
#include "Editor.h"
#include "audio/Bitcrusher.h"

namespace audio
{
    juce::AudioProcessorEditor* Processor::createEditor()
    {
        return new gui::Editor(*this);
    }

    juce::AudioProcessor::BusesProperties ProcessorBackEnd::makeBusesProperties()
    {
        BusesProperties bp;
        bp.addBus(true, "Input", ChannelSet::stereo(), true);
        bp.addBus(false, "Output", ChannelSet::stereo(), true);
#if PPDHasSidechain
        if (!juce::JUCEApplicationBase::isStandaloneApp())
        {
            bp.addBus(true, "Sidechain", ChannelSet::stereo(), true);
        }
#endif
        return bp;
    }

    ProcessorBackEnd::ProcessorBackEnd() :
        juce::AudioProcessor(makeBusesProperties()),
        props(),
        sus(*this),
        state(),
        params(*this, state),
        modSys(state, params),
        midiManager(params, state),
#if PPDHasHQ
        oversampler(),
#endif
        meters()
#if PPDHasStereoConfig
        , midSideEnabled(false)
#endif
		, midiVoices(midiManager)
    {
        {
            juce::PropertiesFile::Options options;
            options.applicationName = JucePlugin_Name;
            options.filenameSuffix = ".settings";
            options.folderName = "Mrugalla" + juce::File::getSeparatorString() + JucePlugin_Name;
            options.osxLibrarySubFolder = "Application Support";
            options.commonToAllUsers = false;
            options.ignoreCaseOfKeyNames = false;
            options.doNotSave = false;
            options.millisecondsBeforeSaving = 20;
            options.storageFormat = juce::PropertiesFile::storeAsXML;

            props.setStorageParameters(options);
        }

        startTimerHz(6);
    }

    const String ProcessorBackEnd::getName() const
    {
        return JucePlugin_Name;
    }

    double ProcessorBackEnd::getTailLengthSeconds() const { return 0.; }

    int ProcessorBackEnd::getNumPrograms() { return 1; }

    int ProcessorBackEnd::getCurrentProgram() { return 0; }

    void ProcessorBackEnd::setCurrentProgram(int) {}

    const String ProcessorBackEnd::getProgramName(int) { return {}; }

    void ProcessorBackEnd::changeProgramName(int, const juce::String&) {}

    bool ProcessorBackEnd::canAddBus(bool isInput) const
    {
        if (wrapperType == wrapperType_Standalone)
            return false;

        return PPDHasSidechain ? isInput : false;
    }

    bool ProcessorBackEnd::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        const auto mono = ChannelSet::mono();
        const auto stereo = ChannelSet::stereo();
        
        const auto mainIn = layouts.getMainInputChannelSet();
        const auto mainOut = layouts.getMainOutputChannelSet();

        if (mainIn != mainOut)
            return false;

        if (mainOut != stereo && mainOut != mono)
            return false;

#if PPDHasSidechain
        if (wrapperType != wrapperType_Standalone)
        {
            const auto scIn = layouts.getChannelSet(true, 1);
            if (!scIn.isDisabled())
                if (scIn != stereo && scIn != mono)
                    return false;
        }
#endif
        return true;
    }

    ProcessorBackEnd::AppProps* ProcessorBackEnd::getProps() noexcept
    {
        return &props;
    }

    void ProcessorBackEnd::savePatch()
    {
        params.savePatch(props);
        midiManager.savePatch();
    }

    void ProcessorBackEnd::loadPatch()
    {
        params.loadPatch(props);
        midiManager.loadPatch();
        forcePrepareToPlay();
    }

    bool ProcessorBackEnd::hasEditor() const { return PPDHasEditor; }
    bool ProcessorBackEnd::acceptsMidi() const { return true; }
    bool ProcessorBackEnd::producesMidi() const { return true; }
    bool ProcessorBackEnd::isMidiEffect() const { return false; }

    /////////////////////////////////////////////
    /////////////////////////////////////////////;
    void ProcessorBackEnd::getStateInformation(juce::MemoryBlock& destData)
    {
        savePatch();
        state.savePatch(*this, destData);
    }

    void ProcessorBackEnd::setStateInformation(const void* data, int sizeInBytes)
    {
        state.loadPatch(*this, data, sizeInBytes);
        loadPatch();
    }

    void ProcessorBackEnd::forcePrepareToPlay()
    {
        sus.suspend();
    }

    void ProcessorBackEnd::timerCallback()
    {
#if PPDHasHQ
        const auto ovsrEnabled = params[PID::HQ]->getValModNorm()[0][0] > .5f;
        if (oversampler.isEnabled() != ovsrEnabled)
            forcePrepareToPlay();
#endif
    }

    void ProcessorBackEnd::processBlockBypassed(AudioBuffer& buffer, juce::MidiBuffer&)
    {
        auto mainBus = getBus(true, 0);
        auto mainBuffer = mainBus->getBusBuffer(buffer);

        if (sus.suspendIfNeeded(mainBuffer))
            return;
        const auto numSamples = mainBuffer.getNumSamples();
        if (numSamples == 0)
            return;

        auto samples = mainBuffer.getArrayOfWritePointers();
        const auto constSamples = mainBuffer.getArrayOfReadPointers();
        const auto numChannels = mainBuffer.getNumChannels();

        modSys(numChannels, numSamples);
		
        dryWetMix.processBypass(samples, numChannels, numSamples);
#if PPDHasGainIn
        meters.processIn(constSamples, numChannels, numSamples);
#endif
        meters.processOut(constSamples, numChannels, numSamples);
    }

    // PROCESSOR

    Processor::Processor() :
        ProcessorBackEnd(),
        ringMod()
    {
    }

    void Processor::prepareToPlay(double sampleRate, int maxBlockSize)
    {
        auto latency = 0;
#if PPDHasHQ
        oversampler.setEnabled(params[PID::HQ]->getValue() > .5f);
        oversampler.prepare(sampleRate, maxBlockSize);
        const auto sampleRateUp = oversampler.getFsUp();
        const auto sampleRateUpF = static_cast<float>(sampleRateUp);
        const auto blockSizeUp = oversampler.getBlockSizeUp();
        latency = oversampler.getLatency();
#endif
        const auto sampleRateF = static_cast<float>(sampleRate);
		
        modSys.prepare(sampleRateF, maxBlockSize);
        midiVoices.prepare(blockSizeUp);

		
        ringMod.prepare(sampleRateUpF, blockSizeUp);


        dryWetMix.prepare(sampleRateF, maxBlockSize, latency);
        meters.prepare(sampleRateF, maxBlockSize);
        setLatencySamples(latency);
        sus.prepareToPlay();
    }

    void Processor::processBlock(AudioBuffer& buffer, juce::MidiBuffer& midi)
    {
        const ScopedNoDenormals noDenormals;

        auto mainBus = getBus(true, 0);
        auto mainBuffer = mainBus->getBusBuffer(buffer);
        
        if (sus.suspendIfNeeded(mainBuffer))
            return;

        const auto numSamples = mainBuffer.getNumSamples();
        if (numSamples == 0)
            return;

        midiManager(midi, numSamples);

        const auto samples = mainBuffer.getArrayOfWritePointers();
        const auto constSamples = mainBuffer.getArrayOfReadPointers();
        const auto numChannels = mainBuffer.getNumChannels();

        modSys(numChannels, numSamples);
		
        if (params[PID::Power]->getValModNorm()[0][0] < .5f)
            return processBlockBypassed(buffer, midi);
		
        dryWetMix.saveDry(
            samples,
            numChannels,
            numSamples
#if PPDHasGainIn
            , params[PID::GainIn]->getValModDenorm()
#endif
#if PPDHasPolarity
            , params[PID::Polarity]->getValModNorm()[0][0] > .5f
#endif
#if PPDHasUnityGain && PPDHasGainIn
            , params[PID::UnityGain]->getValModNorm()[0][0] > .5f
#endif
        );

#if PPDHasGainIn
        meters.processIn(constSamples, numChannels, numSamples);
#endif

#if PPDHasStereoConfig
        midSideEnabled = numChannels == 2 && params[PID::StereoConfig]->getValModNorm()[0][0] > .5f;
        if (midSideEnabled)
        {
            encodeMS(samples, numSamples, 0);
#if PPDHasSidechain
            encodeMS(samples, numSamples, 1);
#endif
        } 

#endif

#if PPDHasHQ
            auto resampledBuf = &oversampler.upsample(buffer);
#else
            auto resampledBuf = &buffer;
#endif
            auto resampledMainBuf = mainBus->getBusBuffer(*resampledBuf);
            
#if PPDHasSidechain
        if (wrapperType != wrapperType_Standalone)
        {
            auto scBus = getBus(true, 1);
            if (scBus != nullptr)
                if(scBus->isEnabled())
                {
                    auto scBuffer = scBus->getBusBuffer(*resampledBuf);

                    processBlockCustom(
                        resampledMainBuf.getArrayOfWritePointers(),
                        resampledMainBuf.getNumChannels(),
                        resampledMainBuf.getNumSamples(),
                        scBuffer.getArrayOfWritePointers(),
                        scBuffer.getNumChannels()
                    );
                }
        }
        else
        {

        }
#else
        processBlockCustom
        (
            resampledMainBuf.getArrayOfWritePointers(),
            resampledMainBuf.getNumChannels(),
            resampledMainBuf.getNumSamples()
        );
#endif
            
#if PPDHasHQ
        oversampler.downsample(mainBuffer);
#endif

#if PPDHasStereoConfig
        if (midSideEnabled)
        {
			decodeMS(samples, numSamples, 0);
#if PPDHasSidechain
			encodeMS(samples, numSamples, 1);
#endif
        }
#endif
        
        dryWetMix.processGainOut(samples, numChannels, numSamples,
            params[PID::Gain]->getValModDenorm()
        );
        meters.processOut(constSamples, numChannels, numSamples);
        dryWetMix.processMix(samples, numChannels, numSamples,
            params[PID::Mix]->getValModNorm()
        );

#if JUCE_DEBUG
        for (auto ch = 0; ch < numChannels; ++ch)
        {
            auto smpls = samples[ch];

            for (auto s = 0; s < numSamples; ++s)
            {
                if (smpls[s] > 1.f)
                    smpls[s] = 1.f;
                else if (smpls[s] < -1.f)
                    smpls[s] = -1.f;
            }
        }
#endif
    }

    void Processor::processBlockCustom(float** samples, int numChannels, int numSamples
#if PPDHasSidechain
        , float** samplesSC, int numChannelsSC
#endif
    ) noexcept
    {
        //auto type = (IIR_4P::Type)std::rint(params[PID::FilterType]->getValModDenorm()[0][0]);
        auto freq = params[PID::RingModFreq]->getValModDenorm();
        
        ringMod(samples, numChannels, numSamples, freq);
    }

    void Processor::releaseResources() {}

    void Processor::savePatch()
    {
        ProcessorBackEnd::savePatch();
    }

    void Processor::loadPatch()
    {
        ProcessorBackEnd::loadPatch();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new audio::Processor();
}
