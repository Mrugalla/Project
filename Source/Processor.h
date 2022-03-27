#pragma once

#include "Smooth.h"
#include "State.h"
#include "Param.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

namespace audio
{
    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = Tau * .5f;

    using AudioBuffer = juce::AudioBuffer<float>;
    using SIMD = juce::FloatVectorOperations;
    using Smooth = smooth::Smooth<float>;
}

#include "RealTimePtr.h"

#include "DryWetMix.h"
#include "MidSide.h"
#include "Oversampling.h"
#include "Meter.h"
#include "Rectifier.h"

#include "config.h"

namespace audio
{
    using State = sta::State;
    using Params = param::Params;
    using PID = param::PID;
    using Timer = juce::Timer;

    struct ProcessorBackEnd :
        public juce::AudioProcessor,
        public Timer
    {
        using ChannelSet = juce::AudioChannelSet;
        using AppProps = juce::ApplicationProperties;

        ProcessorBackEnd() :
            juce::AudioProcessor(BusesProperties()
                .withInput("Input", ChannelSet::stereo(), true)
                .withOutput("Output", ChannelSet::stereo(), true)
            ),
            props(),
            state(),
            params(*this, state)
#if PPDHasHQ
            , oversampler()
#endif
            , meters(),
            midSideEnabled(false)
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
#if PPDHasHQ
            startTimerHz(8);
#endif
        }

        const juce::String getName() const override { return JucePlugin_Name; }
        double getTailLengthSeconds() const override { return 0.; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {};
        bool isBusesLayoutSupported(const BusesLayout& layouts) const override
        {
            if (layouts.getMainOutputChannelSet() != ChannelSet::mono()
                && layouts.getMainOutputChannelSet() != ChannelSet::stereo())
                return false;

            return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
        }
        AppProps* getProps() noexcept { return &props; }

        void savePatch()
        {
            params.savePatch();
        }
        void loadPatch()
        {
            params.loadPatch();
            forcePrepareToPlay();
        }

        bool hasEditor() const override { return PPDHasEditor; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }

        /////////////////////////////////////////////
        /////////////////////////////////////////////
        void getStateInformation(juce::MemoryBlock& destData) override
        {
            savePatch();
            state.savePatch(*this, destData);
        }
        void setStateInformation(const void* data, int sizeInBytes) override
        {
            state.loadPatch(*this, data, sizeInBytes);
            loadPatch();
        }

        AppProps props;
        State state;
        Params params;

        DryWetMix dryWetMix;
#if PPDHasHQ
        Oversampler oversampler;
#endif
        Meters meters;

        void forcePrepareToPlay()
        {
            suspendProcessing(true);
            while (!isSuspended()) {}
            prepareToPlay(getSampleRate(), getBlockSize());
            suspendProcessing(false);
        }

        void timerCallback() override
        {
#if PPDHasHQ
            const auto ovsrEnabled = params[PID::HQ]->getValue() > .5f;
            if (oversampler.isEnabled() != ovsrEnabled)
                forcePrepareToPlay();
#endif
        }

        void processBlockBypassed(AudioBuffer& buffer, juce::MidiBuffer&) override
        {
            auto samples = buffer.getArrayOfWritePointers();
            const auto constSamples = buffer.getArrayOfReadPointers();
            const auto numChannels = buffer.getNumChannels();
            const auto numSamples = buffer.getNumSamples();

            dryWetMix.processBypass(samples, numChannels, numSamples);
#if PPDHasGainIn
            meters.processIn(constSamples, numChannels, numSamples);
#endif
            meters.processOut(constSamples, numChannels, numSamples);
        }

    protected:
        AudioBuffer* processBlockStart(AudioBuffer& buffer, juce::MidiBuffer& midi) noexcept
        {
            const auto numSamples = buffer.getNumSamples();
            if (numSamples == 0)
                return nullptr;

            const auto numChannels = buffer.getNumChannels() == 1 ? 1 : 2;

            const auto constSamples = buffer.getArrayOfReadPointers();
            auto samples = buffer.getArrayOfWritePointers();

            if (params[PID::Power]->getValue() < .5f)
            {
                processBlockBypassed(buffer, midi);
                return nullptr;
            }

            dryWetMix.saveDry(
                samples,
                numChannels,
                numSamples,
#if PPDHasGainIn
                params[PID::GainIn]->getValueDenorm(),
#endif
                params[PID::Mix]->getValue(),
                params[PID::Gain]->getValueDenorm(),
                (params[PID::Polarity]->getValue() > .5f ? -1.f : 1.f)
#if PPDHasUnityGain
                , params[PID::UnityGain]->getValue()
#endif
            );
#if PPDHasGainIn
            meters.processIn(constSamples, numChannels, numSamples);
#endif
            midSideEnabled = numChannels == 2 && params[PID::StereoConfig]->getValue() > .5f;
            if (midSideEnabled)
            {
                encodeMS(samples, numSamples);
                {
#if PPDHasHQ
                    return &oversampler.upsample(buffer);
#else
                    return &buffer;
#endif
                }
            }
            else
            {
#if PPDHasHQ
                return &oversampler.upsample(buffer);
#else
                return &buffer;
#endif
            }
        }

        void processBlockEnd(AudioBuffer& buffer) noexcept
        {
#if PPDHasHQ
            oversampler.downsample(buffer);
#endif
            const auto samples = buffer.getArrayOfWritePointers();
            const auto constSamples = buffer.getArrayOfReadPointers();
            const auto numChannels = buffer.getNumChannels();
            const auto numSamples = buffer.getNumSamples();

            if(midSideEnabled)
                decodeMS(samples, numSamples);

            dryWetMix.processOutGain(samples, numChannels, numSamples);
            meters.processOut(constSamples, numChannels, numSamples);
            dryWetMix.processMix(samples, numChannels, numSamples);
        }
    
private:
        bool midSideEnabled;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorBackEnd)
    };

    struct Processor :
        public ProcessorBackEnd
    {
        Processor() :
            ProcessorBackEnd()
        {
        }

        void prepareToPlay(double sampleRate, int maxBlockSize) override
        {
            auto latency = 0;
#if PPDHasHQ
            oversampler.setEnabled(params[PID::HQ]->getValue() > .5f);
            oversampler.prepare(sampleRate, maxBlockSize);
            const auto sampleRateUp = oversampler.getFsUp();
            const auto maxBlockSizeUp = oversampler.getBlockSizeUp();
            latency = oversampler.getLatency();
#endif
            const auto sampleRateF = static_cast<float>(sampleRate);
            
            dryWetMix.prepare(sampleRateF, maxBlockSize, latency);

            meters.prepare(sampleRateF, maxBlockSize);

            setLatencySamples(latency);
        }

        void processBlock(AudioBuffer& buffer, juce::MidiBuffer& midi)
        {
            const juce::ScopedNoDenormals noDenormals;
            
            auto buf = processBlockStart(buffer, midi);
            if (buf == nullptr)
                return;

            processBlockCustom(
                buf->getArrayOfWritePointers(),
                buf->getNumChannels(),
                buf->getNumSamples()
            );

            processBlockEnd(buffer);
        }
        
        void processBlockCustom(float** samples, int numChannels, int numSamples) noexcept
        {
            rectify(samples, numChannels, numSamples);
        }

        void releaseResources() override {}

        void savePatch()
        {
            ProcessorBackEnd::savePatch();
        }
        void loadPatch()
        {
            ProcessorBackEnd::loadPatch();
        }

        juce::AudioProcessorEditor* createEditor() override;
    };
}

#undef HasEditor

#include "configEnd.h"