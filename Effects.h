#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

// Stereo ping-pong-delay med mörkt "tape-aktigt" lågpass i återkopplingen.
class StereoDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        const int size = (int) (sr * 2.0f) + 4;   // upp till 2 s
        for (auto& b : buffer)
            b.assign ((size_t) size, 0.0f);
        writePos = 0;
        lp[0] = lp[1] = 0.0f;
        timeSmoothed.reset (sampleRate, 0.15);
        timeSmoothed.setCurrentAndTargetValue (0.3f * sr);
        // enpols-lågpass vid ca 3.5 kHz
        lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 3500.0f / sr);
    }

    void process (float* l, float* r, int numSamples, float timeMs, float feedback, float mix)
    {
        if (buffer[0].empty())
            return;

        const int size = (int) buffer[0].size();
        timeSmoothed.setTargetValue (juce::jlimit (1.0f, (float) (size - 3), timeMs * 0.001f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            const float delaySamples = timeSmoothed.getNextValue();

            float readPos = (float) writePos - delaySamples;
            while (readPos < 0.0f) readPos += (float) size;
            const int   i0 = (int) readPos;
            const int   i1 = (i0 + 1) % size;
            const float frac = readPos - (float) i0;

            const float dl = buffer[0][(size_t) i0] * (1.0f - frac) + buffer[0][(size_t) i1] * frac;
            const float dr = buffer[1][(size_t) i0] * (1.0f - frac) + buffer[1][(size_t) i1] * frac;

            lp[0] += lpCoeff * (dl - lp[0]);
            lp[1] += lpCoeff * (dr - lp[1]);

            // ping-pong: vänster matas av höger återkoppling och tvärtom
            buffer[0][(size_t) writePos] = l[i] + lp[1] * feedback;
            buffer[1][(size_t) writePos] = r[i] + lp[0] * feedback;

            writePos = (writePos + 1) % size;

            l[i] += dl * mix;
            r[i] += dr * mix;
        }
    }

private:
    std::vector<float> buffer[2];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> timeSmoothed;
    float lp[2] { 0.0f, 0.0f };
    float lpCoeff = 0.3f;
    float sr = 44100.0f;
    int writePos = 0;
};
