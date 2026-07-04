# GIF Stepper

A VST3 / AU plugin that steps through the frames of a GIF in sync with your DAW's BPM.
Useful for adding an animated visual element to screen captures of your production process (WIP) for social media.

## Supported environments

- macOS (Apple Silicon or Intel — either works, since you build it on your own Mac)
- A VST3 host (Ableton Live, Cubase, FL Studio, etc.) or an AU host (GarageBand, Logic Pro)

No prebuilt binaries are distributed. Follow the steps below to build and install the plugin on your own Mac.
The first build downloads and compiles the JUCE framework, so it takes a few minutes.

## Installation

### 1. Get the source

**If you have git:**

```bash
git clone https://github.com/dianzisounds/gif-stepper.git
cd gif-stepper-vst
```

**If you don't use git:**

Click `Code` → `Download ZIP` on this repository's page, then extract it anywhere.

### 2. Run install.sh

**Run this from Terminal.app — do not double-click it in Finder.** Double-clicking an
unsigned script gets blocked by macOS.

Open Terminal, `cd` into the folder you extracted/cloned, and run:

```bash
cd path/to/gif-stepper-vst   # adjust to wherever you put it
bash install.sh
```

If the following aren't installed yet, the script will print instructions and stop.
Follow them, then run `bash install.sh` again.

- **Xcode Command Line Tools** (`xcode-select --install`)
- **cmake** (`brew install cmake`; if you don't have Homebrew, see https://brew.sh)

On success, the plugin is installed to:

- `~/Library/Audio/Plug-Ins/VST3/GIF Stepper.vst3`
- `~/Library/Audio/Plug-Ins/Components/GIF Stepper.component`

### 3. Load it in your DAW

Restart your DAW or trigger a plugin rescan.

- **GarageBand / Logic Pro**: picked up as an Audio Unit (AU)
- **Ableton Live / Cubase / FL Studio, etc.**: picked up as VST3

## Usage

1. Load the plugin on a track
2. Click "Load GIF" and choose a GIF file
3. Pick a Sync Mode: "Free Run" or "Host Sync" (BPM sync)
4. In Host Sync mode, set the Speed multiplier (1/4, 1/2, 1x, 2x, 4x, etc.)

## License

This project is released under the [GNU Affero General Public License v3.0](LICENSE).
It uses the [JUCE](https://juce.com/) framework.
