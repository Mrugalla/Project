#include "Processor.h"
#include "Editor.h"

#include "config.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new audio::Processor();
}

juce::AudioProcessorEditor* audio::Processor::createEditor()
{
    return new gui::Editor(*this);
}

audio::ProcessorBackEnd::ProcessorBackEnd() :
    juce::AudioProcessor(BusesProperties()
        .withInput("Input", ChannelSet::stereo(), true)
        .withOutput("Output", ChannelSet::stereo(), true)
    ),
    props(),
    sus(*this),
    state(),
    params(*this, state),
    macroProcessor(params),
    midiLearn(params, state),
#if PPDHasHQ
    oversampler(),
#endif
    meters(),
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

    startTimerHz(6);
}

const juce::String audio::ProcessorBackEnd::getName() const { return JucePlugin_Name; }

double audio::ProcessorBackEnd::getTailLengthSeconds() const { return 0.; }

int audio::ProcessorBackEnd::getNumPrograms() { return 1; }

int audio::ProcessorBackEnd::getCurrentProgram() { return 0; }

void audio::ProcessorBackEnd::setCurrentProgram(int) {}

const juce::String audio::ProcessorBackEnd::getProgramName(int) { return {}; }

void audio::ProcessorBackEnd::changeProgramName(int, const juce::String&) {}

bool audio::ProcessorBackEnd::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != ChannelSet::mono()
        && layouts.getMainOutputChannelSet() != ChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

audio::ProcessorBackEnd::AppProps* audio::ProcessorBackEnd::getProps() noexcept { return &props; }

void audio::ProcessorBackEnd::savePatch()
{
    params.savePatch(props);
    midiLearn.savePatch();
}

void audio::ProcessorBackEnd::loadPatch()
{
    params.loadPatch(props);
    midiLearn.loadPatch();
    forcePrepareToPlay();
}

bool audio::ProcessorBackEnd::hasEditor() const { return PPDHasEditor; }
bool audio::ProcessorBackEnd::acceptsMidi() const { return true; }
bool audio::ProcessorBackEnd::producesMidi() const { return true; }
bool audio::ProcessorBackEnd::isMidiEffect() const { return false; }

    /////////////////////////////////////////////
    /////////////////////////////////////////////;
void audio::ProcessorBackEnd::getStateInformation(juce::MemoryBlock& destData)
{
    savePatch();
    state.savePatch(*this, destData);
}

void audio::ProcessorBackEnd::setStateInformation(const void* data, int sizeInBytes)
{
    state.loadPatch(*this, data, sizeInBytes);
    loadPatch();
}

void audio::ProcessorBackEnd::forcePrepareToPlay()
{
    sus.suspend();
}

void audio::ProcessorBackEnd::timerCallback()
{
#if PPDHasHQ
    const auto ovsrEnabled = params[PID::HQ]->getValue() > .5f;
    if (oversampler.isEnabled() != ovsrEnabled)
        forcePrepareToPlay();
#endif
}

void audio::ProcessorBackEnd::processBlockBypassed(AudioBuffer& buffer, juce::MidiBuffer&)
{
    macroProcessor();
    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;
    auto samples = buffer.getArrayOfWritePointers();
    const auto constSamples = buffer.getArrayOfReadPointers();
    const auto numChannels = buffer.getNumChannels();

    dryWetMix.processBypass(samples, numChannels, numSamples);
#if PPDHasGainIn
    meters.processIn(constSamples, numChannels, numSamples);
#endif
    meters.processOut(constSamples, numChannels, numSamples);
}

audio::AudioBuffer* audio::ProcessorBackEnd::processBlockStart(AudioBuffer& buffer, juce::MidiBuffer& midi) noexcept
{
    midiLearn(midi);

    macroProcessor();

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return nullptr;

    if (params[PID::Power]->getValue() < .5f)
    {
        processBlockBypassed(buffer, midi);
        return nullptr;
    }

    const auto numChannels = buffer.getNumChannels() == 1 ? 1 : 2;

    const auto constSamples = buffer.getArrayOfReadPointers();
    auto samples = buffer.getArrayOfWritePointers();

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

void audio::ProcessorBackEnd::processBlockEnd(AudioBuffer& buffer) noexcept
{
#if PPDHasHQ
    oversampler.downsample(buffer);
#endif
    const auto samples = buffer.getArrayOfWritePointers();
    const auto constSamples = buffer.getArrayOfReadPointers();
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (midSideEnabled)
        decodeMS(samples, numSamples);

    dryWetMix.processOutGain(samples, numChannels, numSamples);
    meters.processOut(constSamples, numChannels, numSamples);
    dryWetMix.processMix(samples, numChannels, numSamples);
}

audio::Processor::Processor() :
    ProcessorBackEnd()
{
}

void audio::Processor::prepareToPlay(double sampleRate, int maxBlockSize)
{
    auto latency = 0;
#if PPDHasHQ
    oversampler.setEnabled(params[PID::HQ]->getValue() > .5f);
    oversampler.prepare(sampleRate, maxBlockSize);
    //const auto sampleRateUp = oversampler.getFsUp();
    //const auto maxBlockSizeUp = oversampler.getBlockSizeUp();
    latency = oversampler.getLatency();
#endif
    const auto sampleRateF = static_cast<float>(sampleRate);

    dryWetMix.prepare(sampleRateF, maxBlockSize, latency);

    meters.prepare(sampleRateF, maxBlockSize);

    setLatencySamples(latency);

    sus.prepareToPlay();
}

void audio::Processor::processBlock(AudioBuffer& buffer, juce::MidiBuffer& midi)
{
    const juce::ScopedNoDenormals noDenormals;

    if (sus.suspendIfNeeded(buffer))
        return;

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

void audio::Processor::processBlockCustom(float** samples, int numChannels, int numSamples) noexcept
{
    {
        //const auto crushGain = params[PID::CrushGain]->getValModDenorm();

        //crush(samples, numChannels, numSamples, crushGain);
    }
    nullNoiseSynth(samples, numChannels, numSamples);
}

void audio::Processor::releaseResources() {}

void audio::Processor::savePatch()
{
    ProcessorBackEnd::savePatch();
}

void audio::Processor::loadPatch()
{
    ProcessorBackEnd::loadPatch();
}

#include "configEnd.h"