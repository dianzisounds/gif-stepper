#!/usr/bin/env bash
set -e

echo "GIF Stepper - macOS install script"
echo

if ! xcode-select -p >/dev/null 2>&1; then
    echo "Xcode Command Line Tools not found. Install them and re-run this script:"
    echo "  xcode-select --install"
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake not found. Install it via Homebrew:"
    echo "  brew install cmake"
    echo "(If you don't have Homebrew, see https://brew.sh)"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "Building (first run downloads and compiles JUCE, so this takes a few minutes)..."
cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release -j"$(sysctl -n hw.ncpu)"

VST3_SRC="$BUILD_DIR/GIFStepper_artefacts/Release/VST3/GIF Stepper.vst3"
AU_SRC="$BUILD_DIR/GIFStepper_artefacts/Release/AU/GIF Stepper.component"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"

mkdir -p "$VST3_DEST" "$AU_DEST"
cp -R "$VST3_SRC" "$VST3_DEST/"
cp -R "$AU_SRC" "$AU_DEST/"

echo
echo "Install complete."
echo "  VST3: $VST3_DEST/GIF Stepper.vst3"
echo "  AU:   $AU_DEST/GIF Stepper.component"
echo
echo "Restart your DAW (GarageBand / Logic / Ableton, etc.) or trigger a plugin rescan to use it."
