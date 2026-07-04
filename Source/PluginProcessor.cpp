#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GifOverlayWindow.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "speed", "Speed",
        juce::StringArray{ "1/4", "1/2", "1x", "2x", "4x" },
        2));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "syncMode", "Sync Mode", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "syncUnit", "Sync Unit",
        juce::StringArray{ "Beat", "Bar" },
        0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "overlayCount", "Float Copies",
        juce::StringArray{ "x1", "x2", "x3", "x4" },
        0));
    return layout;
}

GIFStepperAudioProcessor::GIFStepperAudioProcessor()
    : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        ),
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    speedParam = parameters.getRawParameterValue("speed");
    syncModeParam = parameters.getRawParameterValue("syncMode");
    syncUnitParam = parameters.getRawParameterValue("syncUnit");
}

GIFStepperAudioProcessor::~GIFStepperAudioProcessor() = default;

void GIFStepperAudioProcessor::prepareToPlay(double sampleRate_, int samplesPerBlock)
{
    (void)samplesPerBlock;
    sampleRate = sampleRate_;
    freeRunSampleCounter = 0.0;
    prepareToPlayCounter.fetch_add(1);
}

void GIFStepperAudioProcessor::releaseResources() {}

void GIFStepperAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    bypassedBlockCounter.fetch_add(1);
    AudioProcessor::processBlockBypassed(buffer, midiMessages);
}

void GIFStepperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    (void)midiMessages;
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processBlockCounter.fetch_add(1);

    int totalFrames = gifLoader.getFrameCount();
    if (totalFrames > 0)
    {
        double speed = getSpeedMultiplierFromChoice(static_cast<int>(speedParam->load()));

        if (syncModeParam->load() >= 0.5f) // Host Sync
        {
            auto ph = getPlayHead();
            if (ph != nullptr)
            {
                auto pos = ph->getPosition();
                if (pos.hasValue())
                {
                    bool isPlaying = pos->getIsPlaying();
                    lastPlayHeadIsPlaying.store(isPlaying);

                    auto ppq = pos->getPpqPosition();
                    if (ppq.hasValue())
                    {
                        double quarterNotePos = *ppq;
                        lastPlayHeadPpq.store(quarterNotePos);

                        if (isPlaying)
                        {
                            double rawFrame;

                            if (syncUnitParam->load() >= 0.5f) // Bar: whole GIF loops (speed) times per bar
                            {
                                double numerator = 4.0, denominator = 4.0;
                                if (auto timeSig = pos->getTimeSignature())
                                {
                                    numerator = timeSig->numerator;
                                    denominator = timeSig->denominator;
                                }
                                double ppqPerBar = numerator * 4.0 / denominator;

                                double barStart;
                                if (auto lastBar = pos->getPpqPositionOfLastBarStart())
                                    barStart = *lastBar;
                                else
                                    barStart = std::floor(quarterNotePos / ppqPerBar) * ppqPerBar;

                                // Bar index keeps multi-bar loops (speed < 1) phase-continuous
                                double barIndex = std::floor(quarterNotePos / ppqPerBar);
                                double phaseInBar = juce::jlimit(0.0, 1.0, (quarterNotePos - barStart) / ppqPerBar);
                                double loopPos = (barIndex + phaseInBar) * speed;
                                rawFrame = (loopPos - std::floor(loopPos)) * totalFrames;
                            }
                            else // Beat: 4 frames per beat at 1x (1 frame per 16th note)
                            {
                                rawFrame = quarterNotePos * speed * 4.0;
                            }

                            int frame = static_cast<int>(std::floor(rawFrame)) % totalFrames;
                            if (frame < 0)
                                frame += totalFrames;

                            currentFrameIndex.store(frame);
                        }
                    }
                }
            }
        }
        else // Free Run
        {
            freeRunSampleCounter += static_cast<double>(buffer.getNumSamples());
            double baseIntervalSamples = sampleRate * 0.1; // 100ms base
            double intervalSamples = baseIntervalSamples / speed;

            while (freeRunSampleCounter >= intervalSamples)
            {
                freeRunSampleCounter -= intervalSamples;
                int next = (currentFrameIndex.load() + 1) % totalFrames;
                currentFrameIndex.store(next);
            }
        }
    }
}

juce::AudioProcessorEditor* GIFStepperAudioProcessor::createEditor()
{
    return new GIFStepperAudioProcessorEditor(*this);
}

bool GIFStepperAudioProcessor::hasEditor() const { return true; }

const juce::String GIFStepperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GIFStepperAudioProcessor::acceptsMidi() const
{
    #if JucePlugin_WantsMidiInput
    return true;
    #else
    return false;
    #endif
}

bool GIFStepperAudioProcessor::producesMidi() const
{
    #if JucePlugin_ProducesMidiOutput
    return true;
    #else
    return false;
    #endif
}

bool GIFStepperAudioProcessor::isMidiEffect() const
{
    #if JucePlugin_IsMidiEffect
    return true;
    #else
    return false;
    #endif
}

double GIFStepperAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int GIFStepperAudioProcessor::getNumPrograms() { return 1; }
int GIFStepperAudioProcessor::getCurrentProgram() { return 0; }
void GIFStepperAudioProcessor::setCurrentProgram(int index) { (void)index; }
const juce::String GIFStepperAudioProcessor::getProgramName(int index) { (void)index; return {}; }
void GIFStepperAudioProcessor::changeProgramName(int index, const juce::String& newName) { (void)index; (void)newName; }

void GIFStepperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GIFStepperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));

        auto gifPath = parameters.state.getProperty("gifPath").toString();
        if (gifPath.isNotEmpty())
        {
            juce::File gifFile(gifPath);
            if (gifFile.existsAsFile())
                loadGifFromFile(gifFile);
        }
    }
}

juce::AudioProcessorValueTreeState& GIFStepperAudioProcessor::getValueTreeState() { return parameters; }
GifLoader& GIFStepperAudioProcessor::getGifLoader() { return gifLoader; }

void GIFStepperAudioProcessor::loadGifFromFile(const juce::File& file, std::function<void(bool)> onComplete)
{
    juce::Thread::launch([this, file, onComplete]()
    {
        bool success = gifLoader.loadGif(file);
        juce::MessageManager::callAsync([this, file, success, onComplete]()
        {
            if (success)
            {
                currentFrameIndex.store(0);
                parameters.state.setProperty("gifPath", file.getFullPathName(), nullptr);
            }
            if (onComplete)
                onComplete(success);
        });
    });
}

void GIFStepperAudioProcessor::setOverlayVisible(bool shouldBeVisible)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (shouldBeVisible && overlayWindow == nullptr)
        overlayWindow = std::make_unique<GifOverlayWindow>(*this);
    else if (!shouldBeVisible)
        overlayWindow.reset();
}

bool GIFStepperAudioProcessor::isOverlayVisible() const noexcept
{
    return overlayWindow != nullptr;
}

double GIFStepperAudioProcessor::getSpeedMultiplierFromChoice(int choiceIndex)
{
    switch (choiceIndex)
    {
        case 0: return 0.25;  // 1/4
        case 1: return 0.5;   // 1/2
        case 2: return 1.0;   // 1x
        case 3: return 2.0;   // 2x
        case 4: return 4.0;   // 4x
        default: return 1.0;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GIFStepperAudioProcessor();
}
