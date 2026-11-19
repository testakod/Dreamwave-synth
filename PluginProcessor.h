#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "Synth.h"
#include "Effects.h"

class DreamwaveProcessor : public juce::AudioProcessor
{
public:
    DreamwaveProcessor();
    ~DreamwaveProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dreamwave Synth"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Egna presets (sparas som .dwpreset-filer)
    static juce::File getUserPresetDir();
    juce::StringArray getUserPresetNames();
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool loadPresetFile (const juce::File& file);
    const juce::String& getCurrentPresetName() const { return currentPresetName; }
    bool isUserPresetActive() const { return userPresetActive; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void resetAllToDefaults();

    SynthParams synthParams;
    juce::String currentPresetName;
    bool userPresetActive = false;
    std::atomic<float> modWheel { 0.0f };

    juce::Synthesiser synth;
    juce::dsp::Chorus<float> chorus;
    StereoDelay delay;
    juce::Reverb reverb;
    float lastMasterGain = 0.7f;
    int currentProgram = 0;

    static constexpr int kNumVoices = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DreamwaveProcessor)
};
