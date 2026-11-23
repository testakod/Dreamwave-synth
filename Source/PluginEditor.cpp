#include "PluginEditor.h"

//==============================================================================
DreamLookAndFeel::DreamLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    dw::pink);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a2166));
    setColour (juce::Slider::thumbColourId,               dw::cyan);
    setColour (juce::Slider::textBoxTextColourId,         dw::text);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,                 dw::dimText);
    setColour (juce::ComboBox::backgroundColourId,        juce::Colour (0xff2f1a52));
    setColour (juce::ComboBox::textColourId,              dw::text);
    setColour (juce::ComboBox::outlineColourId,           juce::Colour (0xff5b3a96));
    setColour (juce::ComboBox::arrowColourId,             dw::cyan);
    setColour (juce::TextButton::buttonColourId,          juce::Colour (0xff2f1a52));
    setColour (juce::TextButton::textColourOffId,         dw::cyan);
    setColour (juce::AlertWindow::backgroundColourId,     juce::Colour (0xff24123f));
    setColour (juce::AlertWindow::textColourId,           dw::text);
    setColour (juce::PopupMenu::backgroundColourId,       juce::Colour (0xff24123f));
    setColour (juce::PopupMenu::textColourId,             dw::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, dw::pink);
    setColour (juce::PopupMenu::highlightedTextColourId,  juce::Colours::white);
}

//==============================================================================
ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& apvts, ParamIndex index, const juce::String& name)
{
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, Panel::colW - 6, 16);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, kParamIds[index], slider);
}

void ParamKnob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

//==============================================================================
ParamChoice::ParamChoice (juce::AudioProcessorValueTreeState& apvts, ParamIndex index, const juce::String& name)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (kParamIds[index])))
        box.addItemList (p->choices, 1);
    addAndMakeVisible (box);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, kParamIds[index], box);
}

void ParamChoice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth() - 6, 24).translated (0, -8));
}

//==============================================================================
void Panel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (dw::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colour (0xff5b3a96).withAlpha (0.6f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    g.setColour (dw::cyan);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), getLocalBounds().removeFromTop (titleH).reduced (pad + 2, 0),
                juce::Justification::centredLeft);
}

void Panel::resized()
{
    auto r = getLocalBounds().withTrimmedTop (titleH).reduced (pad, 0);
    for (auto* c : controls)
        c->setBounds (r.removeFromLeft (colW));
}

//==============================================================================
DreamwaveEditor::DreamwaveEditor (DreamwaveProcessor& p)
    : AudioProcessorEditor (&p), dwProcessor (p)
{
    setLookAndFeel (&laf);

    // --- Rad 0: oscillatorer ---
    auto* osc1 = makePanel ("Osc 1", 0);
    addChoice (*osc1, Osc1Wave, "Wave");
    addKnob   (*osc1, Osc1Oct, "Octave");
    addKnob   (*osc1, Osc1Level, "Level");
    addKnob   (*osc1, PulseWidth, "Pulse W");

    auto* osc2 = makePanel ("Osc 2", 0);
    addChoice (*osc2, Osc2Wave, "Wave");
    addKnob   (*osc2, Osc2Oct, "Octave");
    addKnob   (*osc2, Osc2Detune, "Detune");
    addKnob   (*osc2, Osc2Level, "Level");

    auto* mix = makePanel ("Sub / Noise", 0);
    addKnob (*mix, SubLevel, "Sub");
    addKnob (*mix, NoiseLevel, "Noise");

    auto* uni = makePanel ("Unison", 0);
    addKnob (*uni, Unison, "Voices");
    addKnob (*uni, Spread, "Spread");

    // --- Rad 1: filter + envelopes ---
    auto* filt = makePanel ("Filter", 1);
    addKnob (*filt, Cutoff, "Cutoff");
    addKnob (*filt, Resonance, "Reso");
    addKnob (*filt, FilterEnvAmt, "Env Amt");
    addKnob (*filt, KeyTrack, "Key Trk");

    auto* fenv = makePanel ("Filter Env", 1);
    addKnob (*fenv, FEnvA, "A");
    addKnob (*fenv, FEnvD, "D");
    addKnob (*fenv, FEnvS, "S");
    addKnob (*fenv, FEnvR, "R");

    auto* aenv = makePanel ("Amp Env", 1);
    addKnob (*aenv, AEnvA, "A");
    addKnob (*aenv, AEnvD, "D");
    addKnob (*aenv, AEnvS, "S");
    addKnob (*aenv, AEnvR, "R");

    // --- Rad 2: LFO, drive, chorus, master ---
    auto* lfo = makePanel ("LFO", 2);
    addKnob   (*lfo, LfoRate, "Rate");
    addChoice (*lfo, LfoShape, "Shape");
    addKnob   (*lfo, LfoPitch, "Pitch");
    addKnob   (*lfo, LfoFilter, "Filter");

    auto* drv = makePanel ("Drive", 2);
    addKnob (*drv, Drive, "Drive");

    auto* cho = makePanel ("Chorus", 2);
    addKnob (*cho, ChorusMix, "Mix");
    addKnob (*cho, ChorusRate, "Rate");
    addKnob (*cho, ChorusDepth, "Depth");

    auto* mst = makePanel ("Master", 2);
    addKnob (*mst, Master, "Volume");

    // --- Rad 3: delay + reverb ---
    auto* dly = makePanel ("Delay", 3);
    addKnob (*dly, DelayTime, "Time");
    addKnob (*dly, DelayFeedback, "Feedback");
    addKnob (*dly, DelayMix, "Mix");

    auto* rev = makePanel ("Reverb", 3);
    addKnob (*rev, ReverbSize, "Size");
    addKnob (*rev, ReverbDamp, "Damp");
    addKnob (*rev, ReverbMix, "Mix");

    auto* fm = makePanel ("FM (Osc 2 > Osc 1)", 3);
    addKnob (*fm, FmAmount, "FM Amt");
    addKnob (*fm, FmRatio, "Ratio");
    addKnob (*fm, FmEnv, "FM Env");

    // --- Preset-val, spara, ladda ---
    presetBox.onChange = [this] { onPresetChosen(); };
    addAndMakeVisible (presetBox);
    refreshPresetList();

    saveButton.onClick   = [this] { showSaveDialog(); };
    loadButton.onClick   = [this] { showLoadDialog(); };
    folderButton.onClick = []
    {
        auto dir = DreamwaveProcessor::getUserPresetDir();
        dir.createDirectory();
        dir.startAsProcess();
    };
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (folderButton);

    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);

    setSize (1000, 56 + 4 * (Panel::titleH + Panel::cellH + Panel::pad + 8) + 8);

    // Värden kan byta program eller återställa ett sparat set medan editorn är öppen.
    lastPresetChangeCount = dwProcessor.getPresetChangeCount();
    startTimerHz (4);
}

DreamwaveEditor::~DreamwaveEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DreamwaveEditor::timerCallback()
{
    const int count = dwProcessor.getPresetChangeCount();
    if (count != lastPresetChangeCount)
    {
        lastPresetChangeCount = count;
        refreshPresetList();
    }
}

void DreamwaveEditor::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);

    for (int i = 0; i < dwProcessor.getNumPrograms(); ++i)
        presetBox.addItem (dwProcessor.getProgramName (i), i + 1);

    const auto user = dwProcessor.getUserPresetNames();
    if (user.size() > 0)
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User presets");
        for (int i = 0; i < user.size(); ++i)
            presetBox.addItem (user[i], 1001 + i);
    }

    const int userIdx = dwProcessor.isUserPresetActive() ? user.indexOf (dwProcessor.getCurrentPresetName()) : -1;
    presetBox.setSelectedId (userIdx >= 0 ? 1001 + userIdx : dwProcessor.getCurrentProgram() + 1,
                             juce::dontSendNotification);
}

void DreamwaveEditor::onPresetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id >= 1001)
    {
        const auto names = dwProcessor.getUserPresetNames();
        if (juce::isPositiveAndBelow (id - 1001, names.size()))
            dwProcessor.loadUserPreset (names[id - 1001]);
    }
    else if (id > 0)
    {
        dwProcessor.setCurrentProgram (id - 1);
    }
}

void DreamwaveEditor::showSaveDialog()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name your preset:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", dwProcessor.getCurrentPresetName().isEmpty() ? "My Preset" : dwProcessor.getCurrentPresetName());
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<DreamwaveEditor> safeThis (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safeThis, w] (int result)
    {
        std::unique_ptr<juce::AlertWindow> owner (w);
        if (result != 1 || safeThis == nullptr)
            return;

        const auto name = owner->getTextEditorContents ("name").trim();
        if (name.isNotEmpty() && safeThis->dwProcessor.saveUserPreset (name))
            safeThis->refreshPresetList();
    }), false);
}

void DreamwaveEditor::showLoadDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Load preset", DreamwaveProcessor::getUserPresetDir(), "*.dwpreset");

    juce::Component::SafePointer<DreamwaveEditor> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis] (const juce::FileChooser& fc)
    {
        if (safeThis == nullptr)
            return;
        const auto file = fc.getResult();
        if (file.existsAsFile())
        {
            safeThis->dwProcessor.loadPresetFile (file);
            safeThis->refreshPresetList();
        }
    });
}

Panel* DreamwaveEditor::makePanel (const juce::String& title, int row)
{
    auto* p = panels.add (new Panel (title));
    panelRow.push_back (row);
    addAndMakeVisible (p);
    return p;
}

void DreamwaveEditor::addKnob (Panel& p, ParamIndex idx, const juce::String& name)
{
    p.addControl (std::make_unique<ParamKnob> (dwProcessor.apvts, idx, name));
}

void DreamwaveEditor::addChoice (Panel& p, ParamIndex idx, const juce::String& name)
{
    p.addControl (std::make_unique<ParamChoice> (dwProcessor.apvts, idx, name));
}

void DreamwaveEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (dw::bgTop, 0.0f, 0.0f, dw::bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Titel med neon-känsla
    auto header = getLocalBounds().removeFromTop (56).reduced (16, 0);
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.setColour (dw::pink.withAlpha (0.35f));
    g.drawText ("DREAMWAVE", header.translated (2, 2), juce::Justification::centredLeft);
    g.setColour (dw::pink);
    g.drawText ("DREAMWAVE", header, juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (13.0f));
    g.setColour (dw::cyan);
    g.drawText ("S Y N T H", header.withTrimmedLeft (178), juce::Justification::centredLeft);
}

void DreamwaveEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (56).reduced (16, 12);
    folderButton.setBounds (header.removeFromRight (62));
    header.removeFromRight (6);
    loadButton.setBounds (header.removeFromRight (52));
    header.removeFromRight (6);
    saveButton.setBounds (header.removeFromRight (52));
    header.removeFromRight (10);
    presetBox.setBounds (header.removeFromRight (210));
    presetLabel.setBounds (header.removeFromRight (64));

    area.reduce (12, 0);
    int rowY[4] {};
    const int rowH = Panel::titleH + Panel::cellH + Panel::pad + 8;
    for (int r = 0; r < 4; ++r)
        rowY[r] = area.getY() + r * rowH;

    int rowX[4] { area.getX(), area.getX(), area.getX(), area.getX() };
    for (int i = 0; i < panels.size(); ++i)
    {
        auto* p = panels[i];
        const int r = panelRow[(size_t) i];
        p->setBounds (rowX[r], rowY[r], p->preferredWidth(), p->preferredHeight());
        rowX[r] += p->preferredWidth() + 8;
    }
}
