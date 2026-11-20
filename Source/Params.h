#pragma once
#include <array>
#include <atomic>

// En enda lista som genererar både enum och parameter-ID:n (så de aldrig kommer ur synk).
#define DW_PARAM_LIST(X)                                              \
    X(Osc1Wave, "osc1Wave")       X(Osc1Oct, "osc1Oct")               \
    X(Osc1Level, "osc1Level")     X(PulseWidth, "pulseWidth")         \
    X(Osc2Wave, "osc2Wave")       X(Osc2Oct, "osc2Oct")               \
    X(Osc2Detune, "osc2Detune")   X(Osc2Level, "osc2Level")           \
    X(SubLevel, "subLevel")       X(NoiseLevel, "noiseLevel")         \
    X(Unison, "unison")           X(Spread, "spread")                 \
    X(FmAmount, "fmAmount")       X(FmRatio, "fmRatio")               \
    X(FmEnv, "fmEnv")                                                 \
    X(Cutoff, "cutoff")           X(Resonance, "resonance")           \
    X(FilterEnvAmt, "filterEnvAmt") X(KeyTrack, "keyTrack")           \
    X(FEnvA, "fEnvA") X(FEnvD, "fEnvD") X(FEnvS, "fEnvS") X(FEnvR, "fEnvR") \
    X(AEnvA, "aEnvA") X(AEnvD, "aEnvD") X(AEnvS, "aEnvS") X(AEnvR, "aEnvR") \
    X(LfoRate, "lfoRate")         X(LfoShape, "lfoShape")             \
    X(LfoPitch, "lfoPitch")       X(LfoFilter, "lfoFilter")           \
    X(Drive, "drive")                                                 \
    X(ChorusMix, "chorusMix")     X(ChorusRate, "chorusRate")         \
    X(ChorusDepth, "chorusDepth")                                     \
    X(DelayTime, "delayTime")     X(DelayFeedback, "delayFeedback")   \
    X(DelayMix, "delayMix")                                           \
    X(ReverbSize, "reverbSize")   X(ReverbDamp, "reverbDamp")         \
    X(ReverbMix, "reverbMix")                                         \
    X(Master, "master")

enum ParamIndex
{
#define X(name, id) name,
    DW_PARAM_LIST(X)
#undef X
    NumParams
};

inline constexpr const char* kParamIds[NumParams] = {
#define X(name, id) id,
    DW_PARAM_LIST(X)
#undef X
};

// Pekare till parametrarnas råvärden (trådsäkra läsningar från ljudtråden).
struct SynthParams
{
    std::array<std::atomic<float>*, NumParams> raw{};
    std::atomic<float>* modWheel = nullptr;

    float get (ParamIndex i) const noexcept { return raw[(size_t) i]->load(); }
    int   getInt (ParamIndex i) const noexcept { return (int) (get (i) + 0.5f); }
    float getModWheel() const noexcept { return modWheel != nullptr ? modWheel->load() : 0.0f; }
};
