#include "GifLoader.h"

#if JUCE_WINDOWS
#include <windows.h>
#endif

static juce::Image resizeImageIfNeeded(juce::Image source, int maxSize)
{
    int w = source.getWidth();
    int h = source.getHeight();
    if (w <= maxSize && h <= maxSize)
        return source;

    double scale = juce::jmin(double(maxSize) / w, double(maxSize) / h);
    int newW = static_cast<int>(w * scale);
    int newH = static_cast<int>(h * scale);

    juce::Image resized(juce::Image::ARGB, newW, newH, true);
    juce::Graphics g(resized);
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
    g.drawImageWithin(source, 0, 0, newW, newH, juce::RectanglePlacement::stretchToFit);
    return resized;
}

GifLoader::GifLoader() = default;
GifLoader::~GifLoader() = default;

std::shared_ptr<const GifLoader::FrameList> GifLoader::getFrames() const
{
    juce::SpinLock::ScopedLockType lock(framesLock);
    return frames;
}

void GifLoader::setFrames(std::shared_ptr<const FrameList> newFrames)
{
    {
        juce::SpinLock::ScopedLockType lock(framesLock);
        frames = std::move(newFrames);
    }
    version.fetch_add(1);
}

bool GifLoader::loadGif(const juce::File& file)
{
    loading.store(true);

#if JUCE_WINDOWS
    auto widePath = file.getFullPathName().toWideCharPointer();
    int len = WideCharToMultiByte(CP_THREAD_ACP, 0, widePath, -1, nullptr, 0, nullptr, nullptr);
    std::string narrowPath(len, 0);
    WideCharToMultiByte(CP_THREAD_ACP, 0, widePath, -1, narrowPath.data(), len, nullptr, nullptr);
    const char* path = narrowPath.c_str();
#else
    juce::String pathString = file.getFullPathName();
    const char* path = pathString.toUTF8();
#endif

    gd_GIF* gif = gd_open_gif(path);
    if (!gif)
    {
        loading.store(false);
        return false;
    }

    const int w = gif->width;
    const int h = gif->height;
    const size_t bufferSize = static_cast<size_t>(w * h * 3);
    std::vector<uint8_t> buffer(bufferSize);

    // Decode into a local list; the previous GIF stays visible until the
    // new one is complete, and readers never see a partially built list.
    FrameList newFrames;

    while (gd_get_frame(gif) > 0)
    {
        gd_render_frame(gif, buffer.data());

        // gifdec paints transparent pixels with the background colour, so
        // when this frame declares transparency, key those pixels out.
        bool hasTransparency = gif->gce.transparency != 0;

        juce::Image frame(juce::Image::ARGB, w, h, true);
        juce::Image::BitmapData bitmap(frame, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                size_t srcIdx = static_cast<size_t>((y * w + x) * 3);
                uint8_t rgb[3] = { buffer[srcIdx + 0], buffer[srcIdx + 1], buffer[srcIdx + 2] };

                if (hasTransparency && gd_is_bgcolor(gif, rgb))
                    bitmap.setPixelColour(x, y, juce::Colours::transparentBlack);
                else
                    bitmap.setPixelColour(x, y, juce::Colour(rgb[0], rgb[1], rgb[2]));
            }
        }

        frame = resizeImageIfNeeded(frame, 512);
        newFrames.push_back(std::move(frame));
    }

    gd_close_gif(gif);

    bool success = !newFrames.empty();
    if (success)
        setFrames(std::make_shared<FrameList>(std::move(newFrames)));

    loading.store(false);
    return success;
}

void GifLoader::clear()
{
    setFrames(std::make_shared<FrameList>());
}

int GifLoader::getFrameCount() const noexcept
{
    return static_cast<int>(getFrames()->size());
}

juce::Image GifLoader::getFrame(int index) const
{
    auto list = getFrames();
    if (index < 0 || index >= static_cast<int>(list->size()))
        return {};
    return (*list)[index];
}

bool GifLoader::isLoading() const noexcept
{
    return loading.load();
}
