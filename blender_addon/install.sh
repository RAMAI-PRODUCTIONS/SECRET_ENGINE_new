#!/bin/bash

echo "Secret Engine Level Editor - Installation Script"
echo ""

ADDON_NAME="secret_engine_level_editor"

# Detect OS and set addons directory
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    ADDONS_DIR="$HOME/Library/Application Support/Blender"
else
    # Linux
    ADDONS_DIR="$HOME/.config/blender"
fi

# Find the latest Blender version
BLENDER_VERSION=$(ls -1 "$ADDONS_DIR" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+$' | sort -V | tail -1)

if [ -z "$BLENDER_VERSION" ]; then
    echo "Blender installation not found."
    echo "Please enter your Blender version (e.g., 3.6, 4.0, 4.1):"
    read BLENDER_VERSION
fi

ADDONS_DIR="$ADDONS_DIR/$BLENDER_VERSION/scripts/addons"

if [ ! -d "$ADDONS_DIR" ]; then
    echo "Creating addons directory: $ADDONS_DIR"
    mkdir -p "$ADDONS_DIR"
fi

echo ""
echo "Installing addon to: $ADDONS_DIR/$ADDON_NAME"
echo ""

if [ -d "$ADDONS_DIR/$ADDON_NAME" ]; then
    echo "Removing old version..."
    rm -rf "$ADDONS_DIR/$ADDON_NAME"
fi

echo "Copying addon files..."
cp -r "$(dirname "$0")/$ADDON_NAME" "$ADDONS_DIR/"

echo ""
echo "Installation complete!"
echo ""
echo "Next steps:"
echo "1. Open Blender"
echo "2. Go to Edit > Preferences > Add-ons"
echo "3. Search for 'Secret Engine Level Editor'"
echo "4. Enable the addon by checking the checkbox"
echo ""
