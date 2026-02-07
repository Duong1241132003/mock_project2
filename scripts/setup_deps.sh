#!/bin/bash
# Setup script for MediaPlayerApp dependencies

echo "Installing Dear ImGui and dependencies..."

# Install required packages
sudo apt-get update
sudo apt-get install -y libimgui-dev libglew-dev

# Create imgui directory if using source version
# mkdir -p imgui

# If libimgui-dev doesn't include SDL2 backend, we need to create wrappers
echo ""
echo "Setup complete!"
echo "Run: cd build && cmake .. && make -j$(nproc)"
