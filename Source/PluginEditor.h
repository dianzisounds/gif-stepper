#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class GIFStepperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    GIFStepperAudioProcessorEditor(GIFStepperAudioProcessor&);
    ~GIFStepperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadGifButtonClicked();
    void updateControlsVisibility();

    GIFStepperAudioProcessor& audioProcessor;

    bool controlsVisible = true;
    int lastPaintedFrame = -1;
    int lastPaintedVersion = -1;

    juce::TextButton loadButton{ "Load GIF" };
    juce::ComboBox speedSelector;
    juce::ComboBox syncUnitSelector;
    juce::ComboBox overlayCountSelector;
    juce::ToggleButton syncModeButton{ "Host Sync" };
    juce::ToggleButton overlayButton{ "Float" };
    juce::Label speedLabel{ "Speed", "Speed:" };

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncUnitAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> overlayCountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GIFStepperAudioProcessorEditor)
};
