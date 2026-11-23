// Headless integration test host for Dreamwave Synth.
// Loads the built AU or VST3 like a DAW would, plays MIDI through it and checks the audio.
// Usage: DwIntegrationHost <path to .component or .vst3> [demo.wav]
// Build with -DDREAMWAVE_BUILD_TESTS=ON; see README.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cstdio>

using namespace juce;

static int passes = 0, fails = 0, warns = 0;

static void check (bool ok, const String& what)
{
    std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    (ok ? passes : fails)++;
}

static void warn (const String& what)
{
    std::printf ("  [WARN] %s\n", what.toRawUTF8());
    ++warns;
}

static void info (const String& what) { std::printf ("  %s\n", what.toRawUTF8()); }

static Array<AudioProcessorParameter*> synthParams (AudioPluginInstance& p)
{
    Array<AudioProcessorParameter*> out;
    for (auto* prm : p.getParameters())
    {
        const auto n = prm->getName (64);
        if (n.startsWith ("MIDI CC") || n == "Bypass" || n.containsIgnoreCase ("program"))
            continue;
        out.add (prm);
    }
    return out;
}

static float toDb (float v) { return Decibels::gainToDecibels (v, -200.0f); }

struct Stats
{
    float peak = 0.0f;
    double sumSq = 0.0;
    int64 n = 0;
    bool nonFinite = false;

    void add (const AudioBuffer<float>& b, int start, int num)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            auto* d = b.getReadPointer (ch);
            for (int i = start; i < start + num; ++i)
            {
                const float v = d[i];
                if (! std::isfinite (v)) { nonFinite = true; continue; }
                peak = jmax (peak, std::abs (v));
                sumSq += (double) v * v;
                ++n;
            }
        }
    }
    float rmsDb() const { return n > 0 ? toDb ((float) std::sqrt (sumSq / (double) n)) : -200.0f; }
};

struct Event { int64 at; MidiMessage msg; };

// Renders `seconds` of audio. `segments` splits the result into stats over [from, to) seconds.
struct Render
{
    std::vector<Stats> seg;
    AudioBuffer<float> audio;   // only filled when capture = true
};

static Render render (AudioPluginInstance& p, double sr, int bs, double seconds,
                      std::vector<Event> events, std::vector<std::pair<double, double>> segments,
                      bool capture = false)
{
    std::sort (events.begin(), events.end(), [] (auto& a, auto& b) { return a.at < b.at; });
    const int64 total = (int64) (seconds * sr);
    const int numCh = jmax (2, p.getTotalNumOutputChannels());

    Render r;
    r.seg.resize (segments.size());
    if (capture)
        r.audio.setSize (2, (int) total);

    AudioBuffer<float> buf (numCh, bs);
    MidiBuffer midi;
    size_t ev = 0;

    for (int64 pos = 0; pos < total; pos += bs)
    {
        const int num = (int) jmin ((int64) bs, total - pos);
        buf.setSize (numCh, num, false, false, true);
        buf.clear();
        midi.clear();
        while (ev < events.size() && events[ev].at < pos + num)
        {
            midi.addEvent (events[ev].msg, (int) jmax ((int64) 0, events[ev].at - pos));
            ++ev;
        }

        p.processBlock (buf, midi);

        for (size_t s = 0; s < segments.size(); ++s)
        {
            const int64 a = (int64) (segments[s].first * sr), b = (int64) (segments[s].second * sr);
            const int64 lo = jmax (a, pos), hi = jmin (b, pos + num);
            if (hi > lo)
                r.seg[s].add (buf, (int) (lo - pos), (int) (hi - lo));
        }
        if (capture)
            for (int ch = 0; ch < 2; ++ch)
                r.audio.copyFrom (ch, (int) pos, buf, jmin (ch, numCh - 1), 0, num);
    }
    return r;
}

static std::vector<Event> chord (double sr, std::initializer_list<int> notes, double on, double off, uint8 vel = 100)
{
    std::vector<Event> e;
    for (int n : notes)
    {
        e.push_back ({ (int64) (on * sr),  MidiMessage::noteOn (1, n, vel) });
        e.push_back ({ (int64) (off * sr), MidiMessage::noteOff (1, n) });
    }
    return e;
}

static std::unique_ptr<AudioPluginInstance> load (AudioPluginFormat& fmt, const String& path, double sr, int bs)
{
    OwnedArray<PluginDescription> types;
    fmt.findAllTypesForFile (types, path);
    if (types.isEmpty())
        return {};
    String err;
    auto inst = fmt.createInstanceFromDescription (*types[0], sr, bs, err);
    if (inst == nullptr)
        std::printf ("  load error: %s\n", err.toRawUTF8());
    return inst;
}

static void prepare (AudioPluginInstance& p, double sr, int bs)
{
    p.releaseResources();
    p.setRateAndBufferSizeDetails (sr, bs);
    p.prepareToPlay (sr, bs);
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI gui;

    if (argc < 2)
    {
        std::printf ("usage: DwIntegrationHost <plugin path> [demo.wav]\n");
        return 2;
    }
    const String path = String (CharPointer_UTF8 (argv[1]));
    const bool isAU = path.endsWithIgnoreCase (".component");

    std::unique_ptr<AudioPluginFormat> fmt;
    if (isAU) fmt = std::make_unique<AudioUnitPluginFormat>();
    else      fmt = std::make_unique<VST3PluginFormat>();

   #if JUCE_ARM
    const char* arch = "arm64";
   #else
    const char* arch = "x86_64";
   #endif
    std::printf ("== %s (%s, %s) ==\n", File (path).getFileName().toRawUTF8(), fmt->getName().toRawUTF8(), arch);

    // ---- 1. Load and basic info ----------------------------------------------------------
    std::printf ("\n-- Load\n");
    auto p = load (*fmt, path, 48000.0, 512);
    check (p != nullptr, "plug-in loads");
    if (p == nullptr)
        return 1;

    const auto desc = p->getPluginDescription();
    info ("name: " + desc.name + ", manufacturer: " + desc.manufacturerName + ", version: " + desc.version);
    check (desc.isInstrument, "reports itself as an instrument");
    check (p->getTotalNumInputChannels() == 0 && p->getTotalNumOutputChannels() == 2,
           "bus layout is 0 in / 2 out (got " + String (p->getTotalNumInputChannels()) + " / "
               + String (p->getTotalNumOutputChannels()) + ")");
    check (p->acceptsMidi(), "accepts MIDI");
    check (p->getLatencySamples() == 0, "no reported latency");
    const int numParams = synthParams (*p).size();
    info ("parameters: " + String (numParams) + " synth (" + String (p->getParameters().size()) + " incl. wrapper-added), tail: "
          + String (p->getTailLengthSeconds()) + " s");
    check (numParams == 42, "exposes all 42 synth parameters (" + String (numParams) + ")");

    const int numProgs = p->getNumPrograms();
    StringArray progNames;
    for (int i = 0; i < numProgs; ++i)
        progNames.add (p->getProgramName (i));
    info ("programs: " + progNames.joinIntoString (", "));
    check (numProgs == 12, "12 factory presets visible to the host (" + String (numProgs) + ")");

    // ---- 2. Silence with no input ---------------------------------------------------------
    std::printf ("\n-- Silence\n");
    prepare (*p, 48000.0, 512);
    p->setCurrentProgram (0);
    {
        auto r = render (*p, 48000.0, 512, 1.0, {}, { { 0.0, 1.0 } });
        check (! r.seg[0].nonFinite && r.seg[0].peak < 1.0e-6f, "outputs silence with no MIDI (peak "
               + String (toDb (r.seg[0].peak), 1) + " dB)");
    }

    // ---- 3. Every preset at several sample rates / block sizes ----------------------------
    std::printf ("\n-- Presets x sample rates x block sizes (C-E-G chord, 1.5 s hold + 1.5 s release)\n");
    float globalPeak = 0.0f;
    String loudest;
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int bs : { 32, 333, 512, 2048 })
        {
            prepare (*p, sr, bs);
            int bad = 0;
            String problems;
            for (int prog = 0; prog < numProgs; ++prog)
            {
                p->setCurrentProgram (prog);
                // FM bass / 808 are meant to be played low
                const bool low = progNames[prog].containsIgnoreCase ("bass");
                auto ev = low ? chord (sr, { 36, 43 }, 0.0, 1.5) : chord (sr, { 60, 64, 67 }, 0.0, 1.5);
                auto r = render (*p, sr, bs, 3.0, ev, { { 0.0, 1.5 }, { 0.0, 3.0 } });
                const auto& hold = r.seg[0];
                const auto& all  = r.seg[1];
                if (all.peak > globalPeak) { globalPeak = all.peak; loudest = progNames[prog] + " @ " + String (sr) + "/" + String (bs); }

                if (all.nonFinite)            { ++bad; problems << progNames[prog] << ": NaN/Inf; "; }
                else if (all.peak > 4.0f)     { ++bad; problems << progNames[prog] << ": peak " << toDb (all.peak) << " dB; "; }
                else if (hold.rmsDb() < -50.0f) { ++bad; problems << progNames[prog] << ": near-silent (" << hold.rmsDb() << " dB RMS); "; }
            }
            check (bad == 0, String (sr, 0) + " Hz, block " + String (bs) + ": all presets produce sound, finite, no runaway"
                   + (problems.isNotEmpty() ? " -> " + problems : String()));
        }
    info ("loudest peak across all runs: " + String (toDb (globalPeak), 1) + " dBFS (" + loudest + ")");
    if (globalPeak > 1.0f)
        warn ("some preset peaks above 0 dBFS with a 3-note chord at velocity 100 (" + loudest + ")");

    // ---- 4. Tail decays to silence ----------------------------------------------------------
    std::printf ("\n-- Release tail\n");
    prepare (*p, 48000.0, 512);
    for (int prog : { 0, 1, 5 })   // Init, Night Drive Pad, Lost in VHS (long reverb/delay)
    {
        p->setCurrentProgram (prog);
        auto r = render (*p, 48000.0, 512, 16.0, chord (48000.0, { 60, 64, 67 }, 0.0, 1.0), { { 15.0, 16.0 } });
        check (! r.seg[0].nonFinite && toDb (r.seg[0].peak) < -60.0f,
               progNames[prog] + ": tail below -60 dB 15 s after note-off (" + String (toDb (r.seg[0].peak), 1) + " dB)");
    }

    // ---- 5. Velocity ------------------------------------------------------------------------
    std::printf ("\n-- Velocity\n");
    p->setCurrentProgram (0);
    {
        auto soft = render (*p, 48000.0, 512, 1.0, chord (48000.0, { 60 }, 0.0, 0.8, 20),  { { 0.1, 0.8 } });
        prepare (*p, 48000.0, 512);
        auto hard = render (*p, 48000.0, 512, 1.0, chord (48000.0, { 60 }, 0.0, 0.8, 127), { { 0.1, 0.8 } });
        check (hard.seg[0].rmsDb() > soft.seg[0].rmsDb() + 3.0f,
               "velocity 127 is louder than 20 (" + String (soft.seg[0].rmsDb(), 1) + " -> " + String (hard.seg[0].rmsDb(), 1) + " dB)");
    }

    // ---- 6. Voice stealing, pitch bend, mod wheel, all-notes-off ----------------------------
    std::printf ("\n-- Stress: voice stealing, controllers\n");
    prepare (*p, 48000.0, 128);
    for (int prog = 0; prog < numProgs; ++prog)
    {
        p->setCurrentProgram (prog);
        std::vector<Event> ev;
        Random rnd (prog + 1);
        for (int k = 0; k < 40; ++k)   // 40 overlapping notes, 8 voices -> heavy stealing
        {
            const int n = 30 + rnd.nextInt (60);
            const double t = k * 0.03;
            ev.push_back ({ (int64) (t * 48000.0), MidiMessage::noteOn (1, n, (uint8) (1 + rnd.nextInt (127))) });
            ev.push_back ({ (int64) ((t + 0.6) * 48000.0), MidiMessage::noteOff (1, n) });
        }
        for (int k = 0; k <= 20; ++k)
        {
            ev.push_back ({ (int64) (k * 0.05 * 48000.0), MidiMessage::pitchWheel (1, (k % 2) ? 16383 : 0) });
            ev.push_back ({ (int64) (k * 0.05 * 48000.0), MidiMessage::controllerEvent (1, 1, (k * 13) % 128) });
        }
        ev.push_back ({ (int64) (2.0 * 48000.0), MidiMessage::allNotesOff (1) });
        auto r = render (*p, 48000.0, 128, 3.0, ev, { { 0.0, 3.0 } });
        if (r.seg[0].nonFinite || r.seg[0].peak > 8.0f)
            check (false, progNames[prog] + ": stress produced " + (r.seg[0].nonFinite ? String ("NaN/Inf") : "peak " + String (toDb (r.seg[0].peak), 1) + " dB"));
    }
    check (true, "40 overlapping notes + pitch-bend/mod-wheel sweeps on every preset: finite and bounded");
    // reset controllers
    render (*p, 48000.0, 128, 0.01, { { 0, MidiMessage::pitchWheel (1, 8192) }, { 0, MidiMessage::controllerEvent (1, 1, 0) } }, {});

    // ---- 7. Parameter fuzzing -----------------------------------------------------------------
    std::printf ("\n-- Parameter fuzzing\n");
    {
        prepare (*p, 44100.0, 256);
        Random rnd (1234);
        const auto params = synthParams (*p);
        int bad = 0;
        float maxPeak = 0.0f;
        String worst;
        for (int it = 0; it < 300; ++it)
        {
            const int mode = it < 2 ? it : 2;   // all-min, all-max, then random
            for (auto* prm : params)
                prm->setValue (mode == 0 ? 0.0f : mode == 1 ? 1.0f : rnd.nextFloat());
            auto r = render (*p, 44100.0, 256, 0.25, chord (44100.0, { 40 + rnd.nextInt (50) }, 0.0, 0.2, 127), { { 0.0, 0.25 } });
            if (r.seg[0].nonFinite || r.seg[0].peak > 8.0f) ++bad;
            if (r.seg[0].peak > maxPeak) { maxPeak = r.seg[0].peak; worst = mode == 0 ? "all-min" : mode == 1 ? "all-max" : "random #" + String (it); }
        }
        check (bad == 0, "300 random parameter sets (incl. all-min / all-max): finite, peak <= +18 dB ("
               + String (bad) + " bad, max peak " + String (toDb (maxPeak), 1) + " dB in " + worst + ")");
        if (maxPeak > 1.0f)
            warn ("extreme parameter settings can exceed 0 dBFS (max " + String (toDb (maxPeak), 1) + " dB, " + worst + ")");
    }

    // ---- 8. Automation while playing (zipper / instability) ----------------------------------
    std::printf ("\n-- Automation while a note plays\n");
    {
        prepare (*p, 48000.0, 64);
        p->setCurrentProgram (2);   // Outrun Lead
        const auto params = synthParams (*p);
        AudioBuffer<float> buf (2, 64);
        MidiBuffer midi;
        midi.addEvent (MidiMessage::noteOn (1, 57, (uint8) 110), 0);
        Stats st;
        for (int blk = 0; blk < 48000 / 64 * 2; ++blk)
        {
            const float ph = (float) blk / 200.0f;
            for (auto* prm : params)
                if (prm->getName (32).containsIgnoreCase ("cutoff") || prm->getName (32).containsIgnoreCase ("reso")
                    || prm->getName (32).containsIgnoreCase ("detune") || prm->getName (32).containsIgnoreCase ("drive"))
                    prm->setValue (0.5f + 0.5f * std::sin (ph * (1.0f + (float) prm->getParameterIndex() * 0.1f)));
            buf.clear();
            p->processBlock (buf, midi);
            midi.clear();
            st.add (buf, 0, 64);
        }
        check (! st.nonFinite && st.peak < 4.0f, "sweeping cutoff/reso/detune/drive every 64 samples stays stable (peak "
               + String (toDb (st.peak), 1) + " dB)");
    }

    // ---- 9. State save / restore into a fresh instance ---------------------------------------
    std::printf ("\n-- State round-trip\n");
    {
        prepare (*p, 48000.0, 512);
        p->setCurrentProgram (9);   // DX7 E-Piano
        const auto params = synthParams (*p);
        Random rnd (99);
        for (int k = 0; k < 6; ++k)
            params[rnd.nextInt (params.size())]->setValue (rnd.nextFloat());

        MemoryBlock state;
        p->getStateInformation (state);
        info ("state size: " + String ((int) state.getSize()) + " bytes");

        auto q = load (*fmt, path, 48000.0, 512);
        check (q != nullptr, "second instance loads");
        if (q != nullptr)
        {
            q->setStateInformation (state.getData(), (int) state.getSize());
            int mismatches = 0;
            String which;
            const auto qParams = synthParams (*q);
            for (int i = 0; i < params.size(); ++i)
            {
                // Stepped params: compare the step the plug-in actually uses, not the host's cached raw value.
                const int steps = params[i]->getNumSteps();
                const bool stepped = params[i]->isDiscrete() || steps < 100;
                auto q0 = [&] (float v) { return stepped ? std::round (v * (float) (steps - 1)) : v; };
                const float a = params[i]->getValue(), b = qParams[i]->getValue();
                if (std::abs (a - b) > 1.0e-4f)
                    info ("    " + params[i]->getName (32) + ": before " + String (a, 4) + " (" + params[i]->getText (a, 32)
                          + "), after " + String (b, 4) + " (" + qParams[i]->getText (b, 32) + ")"
                          + (stepped ? ", " + String (steps) + " steps" : String()));
                if (std::abs (q0 (a) - q0 (b)) > 1.0e-4f && params[i]->getText (a, 32) != qParams[i]->getText (b, 32))
                {
                    ++mismatches;
                    which << params[i]->getName (32) << " ";
                }
            }
            check (mismatches == 0, "all " + String (params.size()) + " parameter values restored"
                   + (mismatches ? " (mismatch: " + which + ")" : String()));
            if (isAU)   // AUBase::RestoreState always marks the restored state as a user preset (-1)
                info ("host-visible program after restore: " + String (q->getCurrentProgram()) + " (AU marks restored state as user preset)");
            else
                check (q->getCurrentProgram() == p->getCurrentProgram(),
                       "current program restored (" + String (q->getCurrentProgram()) + ")");

            // restored instance should sound (and not get reset to the factory preset)
            prepare (*q, 48000.0, 512);
            auto r = render (*q, 48000.0, 512, 1.0, chord (48000.0, { 60, 67 }, 0.0, 0.8), { { 0.0, 1.0 } });
            check (! r.seg[0].nonFinite && r.seg[0].rmsDb() > -50.0f, "restored instance plays ("
                   + String (r.seg[0].rmsDb(), 1) + " dB RMS)");
        }
    }

    // ---- 10. Editor -----------------------------------------------------------------------------
    std::printf ("\n-- Editor\n");
    {
        std::unique_ptr<AudioProcessorEditor> ed (p->createEditorAndMakeActive());
        check (ed != nullptr, "editor opens");
        if (ed != nullptr)
        {
            info ("editor size: " + String (ed->getWidth()) + " x " + String (ed->getHeight()));
            check (ed->getWidth() > 100 && ed->getHeight() > 100, "editor has a sensible size");
            // program change from the host while the editor is open
            for (int i = 0; i < numProgs; ++i)
                p->setCurrentProgram (i);
            MessageManager::getInstance()->runDispatchLoopUntil (600);
            ed.reset();   // the editor's destructor tells the processor
            check (true, "host program changes with editor open, then editor closes cleanly");
        }
    }

    // ---- 11. CPU -------------------------------------------------------------------------------
    std::printf ("\n-- CPU (48 kHz, block 512, 8 held notes)\n");
    {
        prepare (*p, 48000.0, 512);
        for (int prog : { 0, 7, 9 })   // Init, String Ensemble (7 unison), DX7 E-Piano (FM)
        {
            p->setCurrentProgram (prog);
            const double secs = 10.0;
            const auto t0 = Time::getMillisecondCounterHiRes();
            render (*p, 48000.0, 512, secs, chord (48000.0, { 48, 52, 55, 59, 60, 64, 67, 71 }, 0.0, secs), {});
            const double ms = Time::getMillisecondCounterHiRes() - t0;
            const double rt = secs * 1000.0 / ms;
            info (progNames[prog] + ": " + String (rt, 1) + "x realtime (" + String (100.0 / rt, 2) + "% of one core)");
            check (rt > 4.0, progNames[prog] + " runs comfortably in realtime");
        }
    }

    // ---- 12. Demo render ----------------------------------------------------------------------
    if (argc >= 3)
    {
        std::printf ("\n-- Demo WAV\n");
        const double sr = 48000.0;
        prepare (*p, sr, 512);
        AudioBuffer<float> all (2, 0);
        for (int prog = 0; prog < numProgs; ++prog)
        {
            p->setCurrentProgram (prog);
            const bool low = progNames[prog].containsIgnoreCase ("bass");
            std::vector<Event> ev;
            const int base = low ? 36 : 60;
            const int phrase[] = { 0, 7, 12, 3, 10, 7 };
            for (int k = 0; k < 6; ++k)
            {
                ev.push_back ({ (int64) (k * 0.3 * sr), MidiMessage::noteOn (1, base + phrase[k], (uint8) 100) });
                ev.push_back ({ (int64) ((k * 0.3 + 0.25) * sr), MidiMessage::noteOff (1, base + phrase[k]) });
            }
            for (int n : { base, base + 3, base + 7 })
            {
                ev.push_back ({ (int64) (2.0 * sr), MidiMessage::noteOn (1, n, (uint8) 90) });
                ev.push_back ({ (int64) (3.6 * sr), MidiMessage::noteOff (1, n) });
            }
            auto r = render (*p, sr, 512, 5.0, ev, {}, true);
            const int start = all.getNumSamples();
            all.setSize (2, start + r.audio.getNumSamples(), true, true);
            for (int ch = 0; ch < 2; ++ch)
                all.copyFrom (ch, start, r.audio, ch, 0, r.audio.getNumSamples());
            info (String (prog + 1) + ". " + progNames[prog] + " @ " + String (start / sr, 0) + " s");
        }
        File out { String (CharPointer_UTF8 (argv[2])) };
        out.deleteFile();
        WavAudioFormat wav;
        std::unique_ptr<OutputStream> os (out.createOutputStream());
        auto writer = os != nullptr ? wav.createWriterFor (os, AudioFormatWriterOptions{}.withSampleRate (sr)
                                                                                         .withNumChannels (2)
                                                                                         .withBitsPerSample (24))
                                    : nullptr;
        if (writer != nullptr)
        {
            writer->writeFromAudioSampleBuffer (all, 0, all.getNumSamples());
            check (true, "wrote " + out.getFileName() + " (" + String (all.getNumSamples() / sr, 0) + " s)");
        }
        else
            check (false, "could not write demo WAV");
    }

    p.reset();
    std::printf ("\n== %d passed, %d failed, %d warnings ==\n", passes, fails, warns);
    return fails == 0 ? 0 : 1;
}
