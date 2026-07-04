#include "PluginEditor.h"

GIFStepperAudioProcessorEditor::GIFStepperAudioProcessorEditor(GIFStepperAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    loadButton.onClick = [this] { loadGifButtonClicked(); };
    addAndMakeVisible(loadButton);

    speedLabel.attachToComponent(&speedSelector, true);
    speedLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(speedLabel);

    speedSelector.addItemList({ "1/4", "1/2", "1x", "2x", "4x" }, 1);
    addAndMakeVisible(speedSelector);

    syncUnitSelector.addItemList({ "Beat", "Bar" }, 1);
    addAndMakeVisible(syncUnitSelector);

    syncModeButton.setButtonText("Host Sync");
    addAndMakeVisible(syncModeButton);

    overlayButton.setToggleState(audioProcessor.isOverlayVisible(), juce::dontSendNotification);
    overlayButton.onClick = [this] { audioProcessor.setOverlayVisible(overlayButton.getToggleState()); };
    addAndMakeVisible(overlayButton);

    overlayCountSelector.addItemList({ "x1", "x2", "x3", "x4" }, 1);
    addAndMakeVisible(overlayCountSelector);

    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "speed", speedSelector);
    syncUnitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "syncUnit", syncUnitSelector);
    overlayCountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "overlayCount", overlayCountSelector);
    syncModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "syncMode", syncModeButton);

    setSize(560, 400);
    setResizable(true, true);

    startTimerHz(30); // 30fps UI refresh
}

GIFStepperAudioProcessorEditor::~GIFStepperAudioProcessorEditor()
{
    stopTimer();
}

void GIFStepperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(controlsVisible ? juce::Colours::darkgrey : juce::Colours::black);

    auto frame = audioProcessor.getGifLoader().getFrame(audioProcessor.currentFrameIndex.load());
    if (frame.isValid())
    {
        // Without controls the GIF fills the whole editor
        auto imageBounds = controlsVisible
            ? getLocalBounds().reduced(10).withTrimmedTop(60).withTrimmedBottom(30)
            : getLocalBounds();
        g.drawImageWithin(frame, imageBounds.getX(), imageBounds.getY(),
                          imageBounds.getWidth(), imageBounds.getHeight(),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawFittedText("No GIF loaded", getLocalBounds(), juce::Justification::centred, 1);
    }

#if JUCE_DEBUG
    if (controlsVisible)
    {
        // Draw debug info
        g.setColour(juce::Colours::yellow);
        g.setFont(12.0f);
        juce::String debugText = "Frames: " + juce::String(audioProcessor.getGifLoader().getFrameCount())
            + " | Idx: " + juce::String(audioProcessor.currentFrameIndex.load())
            + " | process: " + juce::String(audioProcessor.processBlockCounter.load())
            + " | bypassed: " + juce::String(audioProcessor.bypassedBlockCounter.load())
            + " | prepare: " + juce::String(audioProcessor.prepareToPlayCounter.load())
            + " | Sync: " + juce::String(audioProcessor.getValueTreeState().getRawParameterValue("syncMode")->load() >= 0.5f ? "ON" : "OFF")
            + " | Playing: " + juce::String(audioProcessor.lastPlayHeadIsPlaying.load() ? "YES" : "NO")
            + " | PPQ: " + juce::String(audioProcessor.lastPlayHeadPpq.load(), 2);
        g.drawText(debugText, 10, getHeight() - 25, getWidth() - 20, 20, juce::Justification::left);
    }
#endif
}

void GIFStepperAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto topBar = bounds.removeFromTop(30);

    loadButton.setBounds(topBar.removeFromLeft(90));
    topBar.removeFromLeft(60); // space for the speed label attached to the left

    speedSelector.setBounds(topBar.removeFromLeft(70));
    topBar.removeFromLeft(10);

    syncUnitSelector.setBounds(topBar.removeFromLeft(70));
    topBar.removeFromLeft(10);

    syncModeButton.setBounds(topBar.removeFromLeft(90));
    overlayButton.setBounds(topBar.removeFromLeft(65));
    overlayCountSelector.setBounds(topBar.removeFromLeft(60));
}

void GIFStepperAudioProcessorEditor::timerCallback()
{
    updateControlsVisibility();

    // Only repaint when the displayed frame actually changed; repainting
    // 30x/sec regardless wastes a lot of CPU (especially while stopped).
    int frame = audioProcessor.currentFrameIndex.load();
    int loaderVersion = audioProcessor.getGifLoader().getVersion();

    bool changed = frame != lastPaintedFrame || loaderVersion != lastPaintedVersion;
#if JUCE_DEBUG
    changed = changed || controlsVisible; // keep the debug line live
#endif

    if (changed)
    {
        lastPaintedFrame = frame;
        lastPaintedVersion = loaderVersion;
        repaint();
    }
}

void GIFStepperAudioProcessorEditor::updateControlsVisibility()
{
    // The overlay can be closed from its own window (double-click),
    // so keep the toggle in sync.
    overlayButton.setToggleState(audioProcessor.isOverlayVisible(), juce::dontSendNotification);

    // Show the UI while hovered, or whenever no GIF is loaded yet;
    // otherwise show only the GIF on a plain black background.
    bool shouldShow = isMouseOver(true)
        || audioProcessor.getGifLoader().getFrameCount() <= 0;

    if (shouldShow == controlsVisible)
        return;

    controlsVisible = shouldShow;
    loadButton.setVisible(controlsVisible);
    speedLabel.setVisible(controlsVisible);
    speedSelector.setVisible(controlsVisible);
    syncUnitSelector.setVisible(controlsVisible);
    syncModeButton.setVisible(controlsVisible);
    overlayButton.setVisible(controlsVisible);
    overlayCountSelector.setVisible(controlsVisible);
    repaint(); // background/layout changes with visibility
}

void GIFStepperAudioProcessorEditor::loadGifButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select a GIF file", juce::File{}, "*.gif");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                loadButton.setEnabled(false);
                juce::Component::SafePointer<GIFStepperAudioProcessorEditor> self(this);
                audioProcessor.loadGifFromFile(file, [self](bool)
                {
                    if (self != nullptr)
                        self->loadButton.setEnabled(true);
                });
            }
            fileChooser.reset();
        });
}
