#!/bin/bash
# Check for freeglut dependency and guide user if it's missing.
if ! dpkg -s freeglut3 >/dev/null 2>&1; then
    echo "--------------------------------------------------------"
    echo "Required library 'freeglut3' is not installed."
    echo "Please run this command to install it:"
    echo ""
    echo "    sudo apt-get update && sudo apt-get install freeglut3"
    echo ""
    echo "--------------------------------------------------------"
    exit 1
fi
# Run the game
./CatAndMouse
