#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"

struct DreamSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
// En polyfonisk röst: 2 oscillatorer med unison (PolyBLEP), sub, brus,
// 24 dB lågpassfilter, filter-/amp-envelope och LFO.
class DreamVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int kMaxUnison = 7;
    static constexpr int kControlRate = 16;   // moduleringar uppdateras var 16:e sampel

    explicit DreamVoice (const SynthParams& p) : params (p) {}

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<DreamSound*> (s) != nullptr;
    }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        ampEnv.setSampleRate (newRate);
        filtEnv.setSampleRate (newRate);

        juce::dsp::ProcessSpec spec { newRate, 512, 2 };
        for (auto& f : filters)
        {
            f.prepare (spec);
            f.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        }
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*,
                    int currentPitchWheelPosition) override
    {
        note = midiNoteNumber;
        vel = 0.4f + 0.6f * velocity;
        baseFreq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        pitchBendSemis = ((float) currentPitchWheelPosition - 8192.0f) / 8192.0f * 2.0f;

        for (int o = 0; o < 2; ++o)
            for (int u = 0; u < kMaxUnison; ++u)
                phase[o][u] = rng.nextFloat();

        subPhase = 0.0f;
        lfoPhase = 0.0f;
        counter = 0;

        if (! ampEnv.isActive())
            for (auto& f : filters)
                f.reset();

        const auto s = readParams();
        applyEnvelopeParams (s);
        ampEnv.noteOn();
        filtEnv.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
            filtEnv.noteOff();
        }
        else
        {
            ampEnv.reset();
            filtEnv.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int newValue) override
    {
        pitchBendSemis = ((float) newValue - 8192.0f) / 8192.0f * 2.0f;   // +/- 2 halvtoner
    }

    void controllerMoved (int, int) override {}   // mod-hjulet läses globalt via SynthParams

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive() || getSampleRate() <= 0.0)
            return;

        const float sr = (float) getSampleRate();
        const auto s = readParams();
        applyEnvelopeParams (s);

        const int numCh = out.getNumChannels();
        float* left  = out.getWritePointer (0, startSample);
        float* right = numCh > 1 ? out.getWritePointer (1, startSample) : nullptr;

        const float uniScale = 1.0f / std::sqrt ((float) s.unison);

        for (int i = 0; i < numSamples; ++i)
        {
            const float ampLevel = ampEnv.getNextSample();
            filtEnvValue = filtEnv.getNextSample();

            if (! ampEnv.isActive())
            {
                clearCurrentNote();
                ampEnv.reset();
                filtEnv.reset();
                break;
            }

            if (counter == 0)
                updateControl (s, sr);
            counter = (counter + 1) % kControlRate;

            // --- oscillatorer med unison ---
            float sumL = 0.0f, sumR = 0.0f;
            for (int u = 0; u < s.unison; ++u)
            {
                phase[0][u] += inc[0][u];  if (phase[0][u] >= 1.0f) phase[0][u] -= 1.0f;
                phase[1][u] += inc[1][u];  if (phase[1][u] >= 1.0f) phase[1][u] -= 1.0f;

                float p1 = phase[0][u];
                if (fmIndex > 0.0f)   // FM: osc 2 (sinus) fasmodulerar osc 1
                {
                    p1 += fmIndex * std::sin (juce::MathConstants<float>::twoPi * phase[1][u])
                          / juce::MathConstants<float>::twoPi;
                    p1 -= std::floor (p1);
                }

                const float v = renderOsc (s.wave1, p1, inc[0][u], s.pulseWidth) * s.level1
                              + renderOsc (s.wave2, phase[1][u], inc[1][u], s.pulseWidth) * s.level2;

                sumL += v * gainL[u];
                sumR += v * gainR[u];
            }
            sumL *= uniScale;
            sumR *= uniScale;

            // --- sub (sinus, en oktav ner) och brus, i mitten ---
            subPhase += subInc;  if (subPhase >= 1.0f) subPhase -= 1.0f;
            const float center = std::sin (juce::MathConstants<float>::twoPi * subPhase) * s.sub
                               + (rng.nextFloat() * 2.0f - 1.0f) * s.noise;
            sumL += center * 0.7071f;
            sumR += center * 0.7071f;

            // --- 24 dB lågpass ---
            float fl = filters[1].processSample (0, filters[0].processSample (0, sumL));
            float fr = filters[1].processSample (1, filters[0].processSample (1, sumR));

            const float g = ampLevel * vel * 0.22f;
            fl *= g;
            fr *= g;

            if (right != nullptr)
            {
                left[i]  += fl;
                right[i] += fr;
            }
            else
            {
                left[i] += 0.5f * (fl + fr);
            }
        }
    }

private:
    //==========================================================================
    struct Snapshot
    {
        int wave1, oct1, wave2, oct2, unison, lfoShape;
        float level1, level2, pulseWidth, detune, sub, noise, spread;
        float cutoff, reso, envAmt, keyTrack;
        float lfoRate, lfoPitch, lfoFilter, fmAmt, fmRatio, fmEnv;
        juce::ADSR::Parameters fEnv, aEnv;
    };

    Snapshot readParams() const
    {
        Snapshot s;
        s.wave1      = params.getInt (Osc1Wave);
        s.oct1       = params.getInt (Osc1Oct);
        s.level1     = params.get (Osc1Level);
        s.pulseWidth = params.get (PulseWidth);
        s.wave2      = params.getInt (Osc2Wave);
        s.oct2       = params.getInt (Osc2Oct);
        s.detune     = params.get (Osc2Detune);
        s.level2     = params.get (Osc2Level);
        s.sub        = params.get (SubLevel);
        s.noise      = params.get (NoiseLevel);
        s.unison     = juce::jlimit (1, kMaxUnison, params.getInt (Unison));
        s.spread     = params.get (Spread);
        s.cutoff     = params.get (Cutoff);
        s.reso       = params.get (Resonance);
        s.envAmt     = params.get (FilterEnvAmt);
        s.keyTrack   = params.get (KeyTrack);
        s.lfoRate    = params.get (LfoRate);
        s.lfoShape   = params.getInt (LfoShape);
        s.lfoPitch   = params.get (LfoPitch);
        s.lfoFilter  = params.get (LfoFilter);
        s.fmAmt      = params.get (FmAmount);
        s.fmRatio    = params.get (FmRatio);
        s.fmEnv      = params.get (FmEnv);
        s.fEnv = { params.get (FEnvA), params.get (FEnvD), params.get (FEnvS), params.get (FEnvR) };
        s.aEnv = { params.get (AEnvA), params.get (AEnvD), params.get (AEnvS), params.get (AEnvR) };
        return s;
    }

    void applyEnvelopeParams (const Snapshot& s)
    {
        ampEnv.setParameters (s.aEnv);
        filtEnv.setParameters (s.fEnv);
    }

    //==========================================================================
    static float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    static float renderOsc (int wave, float p, float dt, float pw) noexcept
    {
        switch (wave)
        {
            case 0:   // saw
                return (2.0f * p - 1.0f) - polyBlep (p, dt);

            case 1:   // pulse
            {
                float v = p < pw ? 1.0f : -1.0f;
                v += polyBlep (p, dt);
                float q = p - pw;  if (q < 0.0f) q += 1.0f;
                v -= polyBlep (q, dt);
                return v;
            }
            case 2:   // triangle
                return 4.0f * std::abs (p - 0.5f) - 1.0f;

            default:  // sine
                return std::sin (juce::MathConstants<float>::twoPi * p);
        }
    }

    static float lfoWave (int shape, float p) noexcept
    {
        switch (shape)
        {
            case 0:  return std::sin (juce::MathConstants<float>::twoPi * p);
            case 1:  return 4.0f * std::abs (p - 0.5f) - 1.0f;
            case 2:  return 2.0f * p - 1.0f;
            default: return p < 0.5f ? 1.0f : -1.0f;
        }
    }

    void updateControl (const Snapshot& s, float sr)
    {
        // LFO
        lfoPhase += s.lfoRate * (float) kControlRate / sr;
        lfoPhase -= std::floor (lfoPhase);
        const float lfo = lfoWave (s.lfoShape, lfoPhase);

        // Tonhöjd: bend + vibrato (LFO + mod-hjul)
        const float vibratoCents = (s.lfoPitch + params.getModWheel() * 40.0f) * lfo;
        const float semis = pitchBendSemis + vibratoCents / 100.0f;
        const float pitchMult = std::exp2 (semis / 12.0f);
        const float oct1 = std::exp2 ((float) s.oct1);
        const float oct2 = std::exp2 ((float) s.oct2);
        const float maxInc = 0.45f;
        const bool fm = s.fmAmt > 0.001f;
        const float ratio = fm ? s.fmRatio : 1.0f;
        fmIndex = fm ? s.fmAmt * (1.0f - s.fmEnv + s.fmEnv * juce::jlimit (0.0f, 1.0f, filtEnvValue)) : 0.0f;

        for (int u = 0; u < s.unison; ++u)
        {
            const float x = s.unison > 1 ? ((float) u / (float) (s.unison - 1)) * 2.0f - 1.0f : 0.0f;
            const float cents = x * s.spread;

            inc[0][u] = juce::jmin (maxInc, baseFreq * pitchMult * oct1 * std::exp2 (cents / 1200.0f) / sr);
            inc[1][u] = juce::jmin (maxInc, baseFreq * pitchMult * oct2 * ratio * std::exp2 ((cents + s.detune) / 1200.0f) / sr);

            const float angle = (x * 0.8f + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            gainL[u] = std::cos (angle);
            gainR[u] = std::sin (angle);
        }
        subInc = juce::jmin (maxInc, baseFreq * pitchMult * 0.5f / sr);

        // Filter
        const float octaves = s.envAmt * filtEnvValue
                            + s.lfoFilter * lfo
                            + s.keyTrack * ((float) note - 60.0f) / 12.0f;
        const float maxCut = juce::jmin (20000.0f, sr * 0.45f);
        const float cutoff = juce::jlimit (20.0f, maxCut, s.cutoff * std::exp2 (octaves));

        filters[0].setCutoffFrequency (cutoff);
        filters[0].setResonance (s.reso);
        filters[1].setCutoffFrequency (cutoff);
        filters[1].setResonance (0.707f);
    }

    //==========================================================================
    const SynthParams& params;

    juce::ADSR ampEnv, filtEnv;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> filters;
    juce::Random rng;

    float phase[2][kMaxUnison] {};
    float inc[2][kMaxUnison] {};
    float gainL[kMaxUnison] {}, gainR[kMaxUnison] {};
    float subPhase = 0.0f, subInc = 0.0f;
    float lfoPhase = 0.0f;
    float filtEnvValue = 0.0f, fmIndex = 0.0f;
    float baseFreq = 440.0f, pitchBendSemis = 0.0f, vel = 1.0f;
    int note = 60, counter = 0;
};
