#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Borderless always-on-top desktop window that shows only the GIF with a
// per-pixel transparent background. Owned by the processor so it survives
// the plugin editor being closed. Drag to move, corner to resize,
// double-click to close.
class GifOverlayWindow : public juce::Component,
                         private juce::Timer
{
public:
    explicit GifOverlayWindow(GIFStepperAudioProcessor& p)
        : processor(p)
    {
        setOpaque(false);
        setAlwaysOnTop(true);
        lastCopyCount = getCopyCount();
        setSize(320 * lastCopyCount, 320);

        constrainer.setMinimumSize(80, 80);
        resizer = std::make_unique<juce::ResizableCornerComponent>(this, &constrainer);
        addAndMakeVisible(*resizer);

        addToDesktop(juce::ComponentPeer::windowIsSemiTransparent);

        if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        {
            auto area = display->userArea;
            setTopLeftPosition(area.getRight() - getWidth() - 40, area.getY() + 80);
        }

        setVisible(true);
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto frame = processor.getGifLoader().getFrame(processor.currentFrameIndex.load());
        if (frame.isValid())
        {
            int count = juce::jmax(1, lastCopyCount);
            int cellWidth = getWidth() / count;
            for (int i = 0; i < count; ++i)
                g.drawImageWithin(frame, i * cellWidth, 0, cellWidth, getHeight(),
                                  juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
        }
        else
        {
            // Keep a faint placeholder so the window remains findable
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
            g.setColour(juce::Colours::white);
            g.setFont(14.0f);
            g.drawFittedText("No GIF loaded", getLocalBounds(), juce::Justification::centred, 1);
        }
    }

    void resized() override
    {
        if (resizer != nullptr)
            resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragger.startDraggingComponent(this, e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        dragger.dragComponent(this, e, &constrainer);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        // Deleting ourselves from inside a mouse callback is unsafe;
        // ask the owning processor to do it asynchronously.
        juce::Component::SafePointer<GifOverlayWindow> self(this);
        juce::MessageManager::callAsync([self]
        {
            if (self != nullptr)
                self->processor.setOverlayVisible(false);
        });
    }

private:
    void timerCallback() override
    {
        int count = getCopyCount();
        bool countChanged = count != lastCopyCount;
        if (countChanged)
        {
            // Keep the size of each copy; grow/shrink the window sideways
            int cellWidth = getWidth() / juce::jmax(1, lastCopyCount);
            lastCopyCount = count;
            setSize(cellWidth * count, getHeight());
        }

        // Repainting a layered (per-pixel alpha) window re-uploads the whole
        // window bitmap, so only do it when the frame actually changed.
        int frame = processor.currentFrameIndex.load();
        int loaderVersion = processor.getGifLoader().getVersion();
        if (countChanged || frame != lastPaintedFrame || loaderVersion != lastPaintedVersion)
        {
            lastPaintedFrame = frame;
            lastPaintedVersion = loaderVersion;
            repaint();
        }
    }

    int getCopyCount() const
    {
        return 1 + static_cast<int>(
            processor.getValueTreeState().getRawParameterValue("overlayCount")->load());
    }

    GIFStepperAudioProcessor& processor;
    int lastCopyCount = 1;
    int lastPaintedFrame = -1;
    int lastPaintedVersion = -1;
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableCornerComponent> resizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GifOverlayWindow)
};
