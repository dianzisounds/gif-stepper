#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <memory>
#include <atomic>

extern "C" {
#include "gifdec.h"
}

class GifLoader
{
public:
    using FrameList = std::vector<juce::Image>;

    GifLoader();
    ~GifLoader();

    bool loadGif(const juce::File& file);
    void clear();

    int getFrameCount() const noexcept;
    juce::Image getFrame(int index) const;
    bool isLoading() const noexcept;

    // Incremented whenever the frame list is replaced, so views can tell
    // "same index but different GIF" apart from "nothing changed".
    int getVersion() const noexcept { return version.load(); }

private:
    // Snapshot accessor so readers on the GUI/audio threads never observe
    // a frame list mid-mutation while a load is in progress.
    std::shared_ptr<const FrameList> getFrames() const;
    void setFrames(std::shared_ptr<const FrameList> newFrames);

    std::shared_ptr<const FrameList> frames{ std::make_shared<FrameList>() };
    mutable juce::SpinLock framesLock;
    std::atomic<bool> loading{ false };
    std::atomic<int> version{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GifLoader)
};
