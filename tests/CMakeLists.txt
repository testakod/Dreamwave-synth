juce_add_console_app(DwIntegrationHost PRODUCT_NAME "DwIntegrationHost")

target_sources(DwIntegrationHost PRIVATE IntegrationHost.cpp)

target_compile_definitions(DwIntegrationHost PRIVATE
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(DwIntegrationHost PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    "-framework CoreAudioKit")   # AUGenericView, used by JUCE's AU hosting
