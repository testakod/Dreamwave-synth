#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace dw
{
    // Synthwave-palett
    const juce::Colour bgTop     { 0xff1a0b2e };
    const juce::Colour bgBottom  { 0xff0a0614 };
    const juce::Colour panel     { 0xff24123f };
    const juce::Colour pink      { 0xffff2a9d };
    const juce::Colour cyan      { 0xff2de2e6 };
    const juce::Colour text      { 0xffe9defc };
    const juce::Colour dimText   { 0xff9b86c4 };
}

class DreamLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DreamLookAndFeel();
};

//==============================================================================
// Ratt + etikett kopplad till en parameter
class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& apvts, ParamIndex index, const juce::String& name);
    void resized() override;

private:
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Rullista + etikett kopplad till en valparameter
class ParamChoice : public juce::Component
{
public:
    ParamChoice (juce::AudioProcessorValueTreeState& apvts, ParamIndex index, const juce::String& name);
    void resized() override;

private:
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// Panel med rubrik och ett rutnät av kontroller
class Panel : public juce::Component
{
public:
    static constexpr int colW = 74, cellH = 112, titleH = 24, pad = 8;

    explicit Panel (const juce::String& titleText) : title (titleText) {}

    void addControl (std::unique_ptr<juce::Component> c)
    {
        addAndMakeVisible (*c);
        controls.add (c.release());
    }

    int preferredWidth() const  { return controls.size() * colW + pad * 2; }
    int preferredHeight() const { return titleH + cellH + pad; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::String title;
    juce::OwnedArray<juce::Component> controls;
};

//==============================================================================
class DreamwaveEditor : public juce::AudioProcessorEditor
{
public:
    explicit DreamwaveEditor (DreamwaveProcessor&);
    ~DreamwaveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Panel* makePanel (const juce::String& title, int row);
    void refreshPresetList();
    void onPresetChosen();
    void showSaveDialog();
    void showLoadDialog();
    void addKnob (Panel& p, ParamIndex idx, const juce::String& name);
    void addChoice (Panel& p, ParamIndex idx, const juce::String& name);

    DreamwaveProcessor& processor;
    DreamLookAndFeel laf;

    juce::ComboBox presetBox;
    juce::Label presetLabel;
    juce::TextButton saveButton { "Save" }, loadButton { "Load" }, folderButton { "Folder" };
    std::unique_ptr<juce::FileChooser> chooser;

    juce::OwnedArray<Panel> panels;
    std::vector<int> panelRow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DreamwaveEditor)
};
