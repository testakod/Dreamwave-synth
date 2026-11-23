# Dreamwave Synth (AU + VST3, macOS)

Polyfonisk synt (8 röster) för dreamwave/synthwave. Byggd med JUCE 8.

## Bygga (Mac)

1. Installera Xcode Command Line Tools: `xcode-select --install`
2. Installera CMake: `brew install cmake`  (eller från cmake.org)
3. I den här mappen: `./build.sh`

Första bygget hämtar JUCE automatiskt (kräver internet) och tar några minuter.
Pluginen kopieras automatiskt:
- VST3 → `~/Library/Audio/Plug-Ins/VST3/Dreamwave Synth.vst3`
- Audio Unit → `~/Library/Audio/Plug-Ins/Components/Dreamwave Synth.component`
En fristående version hamnar i `build/DreamwaveSynth_artefacts/Release/Standalone/`.

Färdiga byggen (AU + VST3, universal) finns också som artefakt under GitHub Actions → *Build macOS plug-ins*.

## Använda i Ableton Live

- **Live 11/12**: Settings → Plug-Ins: slå på **Use Audio Units** och/eller **Use VST3 System Folders**, tryck **Rescan**.
- **Live 10**: använd **Audio Unit**-versionen (Live 10.0 kan inte ladda VST3). Preferences → Plug-Ins → slå på **Use Audio Units**, tryck **Rescan**. På Apple Silicon körs Live 10 under Rosetta – pluginet är universal, så det fungerar.

Dra sedan *Dreamwave Synth* från Plug-ins i webbläsaren till ett MIDI-spår.

Om Live vägrar ladda pluginet (osignerat):

```bash
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/VST3/"Dreamwave Synth.vst3"
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/Components/"Dreamwave Synth.component"
```

## Funktioner

- **Osc 1 / Osc 2**: Saw, Pulse, Triangle, Sine, oktav, detune, nivå, pulsbredd
- **Unison**: 1–7 röster per oscillator, stereo-spridning och detune-spridning
- **Sub** (sinus, en oktav ner) och **brus**
- **Filter**: 24 dB lågpass, resonans, envelope-mängd (±4 oktaver), key tracking
- **Envelopes**: ADSR för filter och volym
- **LFO**: sinus/triangel/saw/fyrkant → tonhöjd (vibrato) och filter. Mod-hjulet (CC1) ökar vibrato
- **Effekter**: Drive, Chorus (Juno-aktig), ping-pong Delay, Reverb, Master
- **FM (DX7-stil)**: Osc 2 (sinus) fasmodulerar Osc 1. *FM Amt* = modulationsindex, *Ratio* = frekvensförhållande (0.25-steg), *FM Env* = hur mycket indexet följer filter-envelopen (ger e-piano-/klockattack). Sätt Osc 1 till Sine och Osc 2 Level till 0 för rena FM-ljud.
- Pitch bend ±2 halvtoner
- **12 fabrikspresets**: Init, Night Drive Pad, Outrun Lead, Pluck Arp, Retro Bass, Lost in VHS, 808 Sub Bass, String Ensemble, Juno Brass, DX7 E-Piano, DX7 Bells, DX7 FM Bass

## Egna presets (spara/ladda)

- **Save**: ge ljudet ett namn, så sparas det som en `.dwpreset`-fil och dyker upp under "User presets" i rullistan.
- **Load**: öppna en `.dwpreset`-fil från valfri plats (t.ex. från en kompis). Den kopieras in i presets-mappen.
- **Folder**: öppnar presets-mappen (`~/Library/Application Support/Dreamwave Synth/Presets`) – radera en fil där för att ta bort ett preset.
- Aktuellt ljud sparas dessutom alltid med ditt Live-set.

## Filer

- `Source/Params.h` – alla parametrar (en lista styr enum och ID:n)
- `Source/Synth.h` – röstmotorn
- `Source/Effects.h` – delay
- `Source/Presets.h` – fabrikspresets
- `Source/PluginProcessor.*` – parametrar, presets, effektkedja
- `Source/PluginEditor.*` – gränssnittet

JUCE är licensierat under AGPLv3/kommersiell licens – gäller om du vill distribuera pluginet.
