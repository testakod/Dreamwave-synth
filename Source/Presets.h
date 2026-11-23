#pragma once
#include <vector>
#include <utility>
#include "Params.h"

// Fabrikspresets: värdena läggs ovanpå standardvärdena.
struct Preset
{
    const char* name;
    std::vector<std::pair<ParamIndex, float>> values;
};

inline const std::vector<Preset>& getFactoryPresets()
{
    // Gemensam grund för FM-ljud: ren sinus-bärvåg, öppet filter, osc 2 = tyst modulator.
    #define DW_FM_BASE { Osc1Wave, 3 }, { Osc1Level, 1.0f }, { Osc2Level, 0 }, { SubLevel, 0 }, { Master, 1.0f }, \
                       { Unison, 1 }, { Spread, 0 }, { Cutoff, 19000 }, { Resonance, 0.7f },    \
                       { FilterEnvAmt, 0 }, { KeyTrack, 0 }

    static const std::vector<Preset> presets
    {
        { "Init (Neon Saws)", {} },

        { "Night Drive Pad", {
            { Unison, 5 }, { Spread, 22 }, { Osc2Detune, 10 }, { SubLevel, 0.3f },
            { Cutoff, 1100 }, { FilterEnvAmt, 1.0f }, { FEnvA, 1.2f }, { FEnvD, 1.5f }, { FEnvS, 0.5f },
            { AEnvA, 0.9f }, { AEnvD, 1.0f }, { AEnvS, 0.8f }, { AEnvR, 2.2f },
            { LfoRate, 0.25f }, { LfoFilter, 0.4f }, { LfoPitch, 4 },
            { ChorusMix, 0.7f }, { ReverbSize, 0.9f }, { ReverbMix, 0.5f }, { DelayMix, 0.2f } } },

        { "Outrun Lead", {
            { Osc2Wave, 1 }, { Osc2Detune, 7 }, { Unison, 3 }, { Spread, 10 }, { SubLevel, 0.1f },
            { Cutoff, 3500 }, { Resonance, 2.0f }, { FilterEnvAmt, 1.2f }, { FEnvD, 0.35f }, { FEnvS, 0.5f },
            { AEnvA, 0.005f }, { AEnvD, 0.4f }, { AEnvS, 0.8f }, { AEnvR, 0.35f },
            { LfoRate, 5.5f }, { LfoPitch, 12 },
            { DelayTime, 375 }, { DelayFeedback, 0.4f }, { DelayMix, 0.35f }, { ReverbMix, 0.3f } } },

        { "Pluck Arp", {
            { Osc2Wave, 1 }, { Osc2Detune, 5 }, { Unison, 2 }, { Spread, 8 }, { SubLevel, 0.0f },
            { Cutoff, 500 }, { Resonance, 1.5f }, { FilterEnvAmt, 3.0f }, { FEnvD, 0.22f }, { FEnvS, 0.0f },
            { AEnvA, 0.003f }, { AEnvD, 0.28f }, { AEnvS, 0.0f }, { AEnvR, 0.3f },
            { DelayTime, 281 }, { DelayFeedback, 0.5f }, { DelayMix, 0.4f }, { ReverbMix, 0.3f } } },

        { "Retro Bass", {
            { Osc1Oct, -1 }, { Osc2Oct, -1 }, { Osc2Wave, 1 }, { Osc2Detune, 4 },
            { Unison, 1 }, { SubLevel, 0.6f }, { Cutoff, 450 }, { Resonance, 1.8f },
            { FilterEnvAmt, 2.2f }, { FEnvD, 0.25f }, { FEnvS, 0.1f },
            { AEnvD, 0.25f }, { AEnvS, 0.8f }, { AEnvR, 0.15f },
            { Drive, 0.3f }, { ChorusMix, 0.2f }, { DelayMix, 0.0f }, { ReverbMix, 0.1f } } },

        { "Lost in VHS", {
            { Osc1Wave, 2 }, { Osc2Wave, 0 }, { Osc2Oct, 1 }, { Osc2Detune, 14 }, { Unison, 4 }, { Spread, 30 },
            { Cutoff, 1800 }, { FilterEnvAmt, 0.5f }, { AEnvA, 0.4f }, { AEnvR, 1.8f },
            { LfoRate, 0.8f }, { LfoPitch, 18 }, { LfoShape, 1 },
            { ChorusMix, 0.9f }, { ChorusRate, 0.35f }, { ChorusDepth, 0.8f },
            { DelayTime, 500 }, { DelayFeedback, 0.5f }, { DelayMix, 0.35f }, { ReverbSize, 0.95f }, { ReverbMix, 0.55f } } },

        // ---- Nya ----
        { "808 Sub Bass", {   // spela i C1–C2
            { Osc1Wave, 3 }, { Osc1Oct, -1 }, { Osc1Level, 1.0f }, { Osc2Level, 0 }, { SubLevel, 0 },
            { Unison, 1 }, { Spread, 0 }, { Cutoff, 1200 }, { Resonance, 0.7f }, { FilterEnvAmt, 0 }, { KeyTrack, 0.3f },
            { AEnvA, 0.002f }, { AEnvD, 1.4f }, { AEnvS, 0.25f }, { AEnvR, 0.2f }, { Master, 1.0f },
            { Drive, 0.45f }, { ChorusMix, 0 }, { DelayMix, 0 }, { ReverbSize, 0.3f }, { ReverbMix, 0.06f } } },

        { "String Ensemble", {
            { Osc2Detune, 12 }, { Unison, 7 }, { Spread, 25 }, { SubLevel, 0 },
            { Cutoff, 3200 }, { FilterEnvAmt, 0.3f }, { KeyTrack, 0.6f }, { FEnvA, 0.5f }, { FEnvD, 1.0f }, { FEnvS, 0.8f },
            { AEnvA, 0.55f }, { AEnvD, 0.8f }, { AEnvS, 0.9f }, { AEnvR, 1.3f },
            { LfoRate, 5.2f }, { LfoPitch, 6 },
            { ChorusMix, 0.8f }, { ChorusRate, 0.45f }, { ChorusDepth, 0.6f },
            { DelayMix, 0 }, { ReverbSize, 0.7f }, { ReverbMix, 0.4f } } },

        { "Juno Brass", {
            { Osc2Wave, 1 }, { Osc2Detune, 5 }, { Osc2Level, 0.6f }, { PulseWidth, 0.45f }, { Unison, 2 }, { Spread, 7 }, { SubLevel, 0.15f },
            { Cutoff, 700 }, { Resonance, 1.2f }, { FilterEnvAmt, 2.5f }, { KeyTrack, 0.5f },
            { FEnvA, 0.1f }, { FEnvD, 0.45f }, { FEnvS, 0.6f }, { FEnvR, 0.3f },
            { AEnvA, 0.05f }, { AEnvD, 0.3f }, { AEnvS, 0.9f }, { AEnvR, 0.25f },
            { Drive, 0.1f }, { ChorusMix, 0.55f }, { DelayMix, 0 }, { ReverbSize, 0.5f }, { ReverbMix, 0.2f } } },

        { "DX7 E-Piano", {
            DW_FM_BASE, { FmAmount, 2.0f }, { FmRatio, 1.0f }, { FmEnv, 0.9f },
            { FEnvA, 0.001f }, { FEnvD, 0.9f }, { FEnvS, 0.1f }, { FEnvR, 0.4f },
            { AEnvA, 0.002f }, { AEnvD, 2.2f }, { AEnvS, 0.2f }, { AEnvR, 0.35f },
            { ChorusMix, 0.45f }, { ChorusRate, 0.5f }, { ChorusDepth, 0.4f },
            { DelayMix, 0 }, { ReverbSize, 0.5f }, { ReverbMix, 0.2f } } },

        { "DX7 Bells", {
            DW_FM_BASE, { FmAmount, 3.5f }, { FmRatio, 3.5f }, { FmEnv, 1.0f },
            { FEnvA, 0.001f }, { FEnvD, 3.0f }, { FEnvS, 0.0f }, { FEnvR, 2.0f },
            { AEnvA, 0.001f }, { AEnvD, 3.5f }, { AEnvS, 0.0f }, { AEnvR, 2.5f },
            { ChorusMix, 0.3f }, { DelayTime, 375 }, { DelayMix, 0.2f }, { ReverbSize, 0.85f }, { ReverbMix, 0.45f } } },

        { "DX7 FM Bass", {
            DW_FM_BASE, { Osc1Oct, -1 }, { Osc2Oct, -1 }, { Osc1Level, 1.0f }, { Cutoff, 6000 },
            { FmAmount, 4.5f }, { FmRatio, 1.0f }, { FmEnv, 1.0f },
            { FEnvA, 0.001f }, { FEnvD, 0.25f }, { FEnvS, 0.15f }, { FEnvR, 0.15f },
            { AEnvA, 0.002f }, { AEnvD, 0.5f }, { AEnvS, 0.7f }, { AEnvR, 0.12f },
            { Drive, 0.1f }, { ChorusMix, 0.15f }, { DelayMix, 0 }, { ReverbSize, 0.3f }, { ReverbMix, 0.1f } } },
    };
    #undef DW_FM_BASE
    return presets;
}
