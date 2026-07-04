#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GifLoader.h"

class GIFStepperAudioProcessor : public juce::AudioProcessor
{
public:
    GIFStepperAudioProcessor();
    ~GIFStepperAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState();
    GifLoader& getGifLoader();

    // Loads asynchronously on a background thread. On success the path is
    // stored in the plugin state so the GIF is restored with the project.
    // onComplete (optional) is called on the message thread.
    void loadGifFromFile(const juce::File& file, std::function<void(bool)> onComplete = {});

    // Message thread only. The overlay outlives the editor so the GIF
    // keeps floating over the DAW while the plugin window is closed.
    void setOverlayVisible(bool shouldBeVisible);
    bool isOverlayVisible() const noexcept;

    std::atomic<int> currentFrameIndex{ 0 };
    std::atomic<int> processBlockCounter{ 0 };
    std::atomic<int> bypassedBlockCounter{ 0 };
    std::atomic<int> prepareToPlayCounter{ 0 };
    std::atomic<double> lastPlayHeadPpq{ -1.0 };
    std::atomic<bool> lastPlayHeadIsPlaying{ false };

    static double getSpeedMultiplierFromChoice(int choiceIndex);

private:
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* syncModeParam = nullptr;
    std::atomic<float>* syncUnitParam = nullptr;

    GifLoader gifLoader;
    std::unique_ptr<juce::Component> overlayWindow;
    double sampleRate = 44100.0;
    double freeRunSampleCounter = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GIFStepperAudioProcessor)
};
