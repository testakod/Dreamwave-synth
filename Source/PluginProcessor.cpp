#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using juce::NormalisableRange;
using juce::ParameterID;

//==============================================================================
// Parametrar
juce::AudioProcessorValueTreeState::ParameterLayout DreamwaveProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&] (ParamIndex idx, const char* name, float lo, float hi, float def,
                         float skewCentre = 0.0f, const char* label = "", float interval = 0.0f)
    {
        NormalisableRange<float> range (lo, hi, interval);
        if (skewCentre > 0.0f)
            range.setSkewForCentre (skewCentre);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParameterID { kParamIds[idx], 1 }, name, range, def,
            juce::AudioParameterFloatAttributes().withLabel (label)));
    };

    auto addInt = [&] (ParamIndex idx, const char* name, int lo, int hi, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterInt> (
            ParameterID { kParamIds[idx], 1 }, name, lo, hi, def));
    };

    auto addChoice = [&] (ParamIndex idx, const char* name, juce::StringArray choices, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            ParameterID { kParamIds[idx], 1 }, name, choices, def));
    };

    const juce::StringArray waves { "Saw", "Pulse", "Tri", "Sine" };

    // Oscillatorer
    addChoice (Osc1Wave,   "Wave",    waves, 0);
    addInt    (Osc1Oct,    "Oct",     -2, 2, 0);
    addFloat  (Osc1Level,  "Level",   0.0f, 1.0f, 0.7f);
    addFloat  (PulseWidth, "PW",      0.05f, 0.95f, 0.5f);
    addChoice (Osc2Wave,   "Wave",    waves, 0);
    addInt    (Osc2Oct,    "Oct",     -2, 2, 0);
    addFloat  (Osc2Detune, "Detune",  -50.0f, 50.0f, 8.0f, 0.0f, " ct");
    addFloat  (Osc2Level,  "Level",   0.0f, 1.0f, 0.5f);
    addFloat  (SubLevel,   "Sub",     0.0f, 1.0f, 0.2f);
    addFloat  (NoiseLevel, "Noise",   0.0f, 1.0f, 0.0f);
    addInt    (Unison,     "Voices",  1, 7, 3);
    addFloat  (Spread,     "Spread",  0.0f, 60.0f, 14.0f, 0.0f, " ct");

    // FM (osc 2 som sinus-modulator för osc 1, DX7-stil)
    addFloat (FmAmount, "FM Amt", 0.0f, 10.0f, 0.0f);
    addFloat (FmRatio,  "Ratio",  0.25f, 14.0f, 1.0f, 2.0f, "", 0.25f);
    addFloat (FmEnv,    "FM Env", 0.0f, 1.0f, 1.0f);

    // Filter
    addFloat (Cutoff,       "Cutoff",    20.0f, 20000.0f, 2200.0f, 1000.0f, " Hz");
    addFloat (Resonance,    "Reso",      0.5f, 10.0f, 0.9f, 2.0f);
    addFloat (FilterEnvAmt, "Env Amt",   -4.0f, 4.0f, 1.5f, 0.0f, " oct");
    addFloat (KeyTrack,     "Key Track", 0.0f, 1.0f, 0.5f);

    addFloat (FEnvA, "Attack",  0.001f, 5.0f, 0.005f, 0.2f, " s");
    addFloat (FEnvD, "Decay",   0.001f, 5.0f, 0.5f,   0.5f, " s");
    addFloat (FEnvS, "Sustain", 0.0f, 1.0f, 0.3f);
    addFloat (FEnvR, "Release", 0.001f, 8.0f, 0.6f,   0.8f, " s");

    addFloat (AEnvA, "Attack",  0.001f, 5.0f, 0.01f, 0.2f, " s");
    addFloat (AEnvD, "Decay",   0.001f, 5.0f, 0.3f,  0.5f, " s");
    addFloat (AEnvS, "Sustain", 0.0f, 1.0f, 0.7f);
    addFloat (AEnvR, "Release", 0.001f, 8.0f, 0.9f,  0.8f, " s");

    // LFO
    addFloat  (LfoRate,   "Rate",   0.05f, 20.0f, 4.5f, 2.0f, " Hz");
    addChoice (LfoShape,  "Shape",  { "Sine", "Tri", "Saw", "Square" }, 0);
    addFloat  (LfoPitch,  "Pitch",  0.0f, 100.0f, 0.0f, 0.0f, " ct");
    addFloat  (LfoFilter, "Filter", 0.0f, 4.0f, 0.0f, 0.0f, " oct");

    // Effekter
    addFloat (Drive,         "Drive",    0.0f, 1.0f, 0.0f);
    addFloat (ChorusMix,     "Mix",      0.0f, 1.0f, 0.5f);
    addFloat (ChorusRate,    "Rate",     0.1f, 5.0f, 0.6f, 0.8f, " Hz");
    addFloat (ChorusDepth,   "Depth",    0.0f, 1.0f, 0.5f);
    addFloat (DelayTime,     "Time",     20.0f, 1000.0f, 375.0f, 300.0f, " ms");
    addFloat (DelayFeedback, "Feedback", 0.0f, 0.9f, 0.35f);
    addFloat (DelayMix,      "Mix",      0.0f, 1.0f, 0.25f);
    addFloat (ReverbSize,    "Size",     0.0f, 1.0f, 0.75f);
    addFloat (ReverbDamp,    "Damp",     0.0f, 1.0f, 0.5f);
    addFloat (ReverbMix,     "Mix",      0.0f, 1.0f, 0.35f);
    addFloat (Master,        "Master",   0.0f, 1.0f, 0.7f);

    return layout;
}

namespace { const std::vector<Preset>& getPresets() { return getFactoryPresets(); } }

//==============================================================================
DreamwaveProcessor::DreamwaveProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < NumParams; ++i)
        synthParams.raw[(size_t) i] = apvts.getRawParameterValue (kParamIds[i]);
    synthParams.modWheel = &modWheel;

    synth.addSound (new DreamSound());
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new DreamVoice (synthParams));
}

void DreamwaveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);

    delay.prepare (sampleRate);

    reverb.setSampleRate (sampleRate);
    reverb.reset();
    lastMasterGain = apvts.getRawParameterValue (kParamIds[Master])->load();
}

bool DreamwaveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DreamwaveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 1)
            modWheel.store ((float) msg.getControllerValue() / 127.0f);
    }

    synth.renderNextBlock (buffer, midi, 0, numSamples);

    if (buffer.getNumChannels() < 2)
        return;

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    // --- Drive (tanh-mättnad) ---
    const float drive = synthParams.get (Drive);
    if (drive > 0.001f)
    {
        const float g = 1.0f + drive * 10.0f;
        const float comp = 1.0f / std::pow (g, 0.35f);
        for (int i = 0; i < numSamples; ++i)
        {
            l[i] = std::tanh (l[i] * g) * comp;
            r[i] = std::tanh (r[i] * g) * comp;
        }
    }

    // --- Chorus ---
    chorus.setRate (synthParams.get (ChorusRate));
    chorus.setDepth (synthParams.get (ChorusDepth));
    chorus.setMix (synthParams.get (ChorusMix));
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        chorus.process (context);
    }

    // --- Delay ---
    delay.process (l, r, numSamples,
                   synthParams.get (DelayTime),
                   synthParams.get (DelayFeedback),
                   synthParams.get (DelayMix));

    // --- Reverb ---
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = synthParams.get (ReverbSize);
        rp.damping    = synthParams.get (ReverbDamp);
        rp.wetLevel   = synthParams.get (ReverbMix) * 0.33f;
        rp.dryLevel   = 0.5f;     // juce::Reverb skalar dry x2, alltså enhetsförstärkning
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (l, r, numSamples);
    }

    // --- Master ---
    const float master = synthParams.get (Master);
    buffer.applyGainRamp (0, numSamples, lastMasterGain, master);
    lastMasterGain = master;
}

//==============================================================================
juce::AudioProcessorEditor* DreamwaveProcessor::createEditor()
{
    return new DreamwaveEditor (*this);
}

int DreamwaveProcessor::getNumPrograms() { return (int) getPresets().size(); }

const juce::String DreamwaveProcessor::getProgramName (int index)
{
    const auto& p = getPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? p[(size_t) index].name : juce::String();
}

void DreamwaveProcessor::setCurrentProgram (int index)
{
    const auto& presets = getPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram = index;
    currentPresetName = presets[(size_t) index].name;
    userPresetActive = false;

    // Först standardvärden, sedan presetens egna.
    resetAllToDefaults();

    for (const auto& [id, value] : presets[(size_t) index].values)
        if (auto* p = apvts.getParameter (kParamIds[id]))
            p->setValueNotifyingHost (p->convertTo0to1 (value));

    ++presetChangeCount;
}

void DreamwaveProcessor::resetAllToDefaults()
{
    for (int i = 0; i < NumParams; ++i)
        if (auto* p = apvts.getParameter (kParamIds[i]))
            p->setValueNotifyingHost (p->getDefaultValue());
}

//==============================================================================
// Egna presets
juce::File DreamwaveProcessor::getUserPresetDir()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    dir = dir.getChildFile ("Application Support");
   #endif
    return dir.getChildFile ("Dreamwave Synth").getChildFile ("Presets");
}

juce::StringArray DreamwaveProcessor::getUserPresetNames()
{
    juce::StringArray names;
    for (auto& f : getUserPresetDir().findChildFiles (juce::File::findFiles, false, "*.dwpreset"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool DreamwaveProcessor::saveUserPreset (const juce::String& name)
{
    auto dir = getUserPresetDir();
    if (! dir.createDirectory().wasOk())
        return false;

    auto file = dir.getChildFile (juce::File::createLegalFileName (name) + ".dwpreset");
    auto state = apvts.copyState();
    state.setProperty ("presetName", name, nullptr);

    if (auto xml = state.createXml())
        if (file.replaceWithText (xml->toString()))
        {
            currentPresetName = file.getFileNameWithoutExtension();
            userPresetActive = true;
            ++presetChangeCount;
            return true;
        }
    return false;
}

bool DreamwaveProcessor::loadPresetFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    resetAllToDefaults();   // så att parametrar som saknas i filen får standardvärde
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    currentPresetName = file.getFileNameWithoutExtension();
    userPresetActive = true;

    // Fil från annan plats: kopiera in i presets-mappen så den dyker upp i listan
    auto dir = getUserPresetDir();
    if (file.getParentDirectory() != dir && dir.createDirectory().wasOk())
        file.copyFileTo (dir.getChildFile (file.getFileName()));

    ++presetChangeCount;
    return true;
}

bool DreamwaveProcessor::loadUserPreset (const juce::String& name)
{
    return loadPresetFile (getUserPresetDir().getChildFile (name + ".dwpreset"));
}

void DreamwaveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("userPreset", userPresetActive, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DreamwaveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("program", 0));
            currentPresetName = state.getProperty ("presetName", "").toString();
            userPresetActive = (bool) state.getProperty ("userPreset", false);
            apvts.replaceState (state);
            ++presetChangeCount;
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DreamwaveProcessor();
}
