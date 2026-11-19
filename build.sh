#!/bin/bash
# Bygger Dreamwave Synth (VST3 + Standalone) och kopierar VST3 till ~/Library/Audio/Plug-Ins/VST3
set -e
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8
echo "Klart! Starta om/skanna om plugins i Ableton Live."
