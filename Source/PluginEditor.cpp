/*
  ==============================================================================
    Aetheri - High-Fidelity Dual-Channel Mastering EQ
    PluginEditor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/Presets.h"
#include "UI/PhaseCorrelationMeter.h"
#include "BinaryData.h"

AetheriAudioProcessorEditor::AetheriAudioProcessorEditor(AetheriAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      inputVU("INPUT"),
      outputVU("OUTPUT"),
      leftChannelStrip("LEFT / MID"),
      rightChannelStrip("RIGHT / SIDE"),
      inputGainKnob("INPUT", Aetheri::AnalogKnob::KnobSize::Medium),
      outputTrimKnob("OUTPUT", Aetheri::AnalogKnob::KnobSize::Medium),
      hpfFreqKnob("HPF", Aetheri::AnalogKnob::KnobSize::Small),
      lpfFreqKnob("LPF", Aetheri::AnalogKnob::KnobSize::Small)
{
    // Load company logo from binary data
    // Note: After rebuilding, BinaryData.h will contain company_logo_png and company_logo_pngSize
    try
    {
        if (BinaryData::company_logo_png != nullptr && BinaryData::company_logo_pngSize > 0)
        {
            companyLogo = juce::ImageCache::getFromMemory(BinaryData::company_logo_png, BinaryData::company_logo_pngSize);
            
            // If cache doesn't work, try direct decode
            if (!companyLogo.isValid())
            {
                juce::MemoryInputStream stream(BinaryData::company_logo_png, BinaryData::company_logo_pngSize, false);
                auto format = juce::ImageFileFormat::findImageFormatForStream(stream);
                if (format != nullptr)
                {
                    stream.setPosition(0);  // Reset stream position
                    companyLogo = format->decodeImage(stream);
                }
            }
        }
    }
    catch (...)
    {
        // If image loading fails, continue without logo
        companyLogo = juce::Image();
    }
    
    // Set custom look and feel
    setLookAndFeel(&lookAndFeel);
    
    // Background nebula visualizer (goes behind everything)
    addAndMakeVisible(nebulaVisualizer);
    nebulaVisualizer.setVisible(true);
    nebulaVisualizer.setAnimating(true);
    nebulaVisualizer.setInterceptsMouseClicks(false, false);  // Don't intercept mouse
    nebulaVisualizer.toBack();  // Ensure it's behind everything
    
    // Main components on top of nebula - make them opaque
    addAndMakeVisible(inputVU);
    inputVU.setOpaque(true);  // Make opaque so it's visible
    addAndMakeVisible(outputVU);
    outputVU.setOpaque(true);  // Make opaque so it's visible
    addAndMakeVisible(leftChannelStrip);
    leftChannelStrip.setOpaque(true);  // Ensure visibility
    addAndMakeVisible(rightChannelStrip);
    rightChannelStrip.setOpaque(true);  // Ensure visibility
    addAndMakeVisible(hysteresisPanel);
    hysteresisPanel.setOpaque(true);  // Ensure visibility
    
    // Global controls
    setupGlobalControls();
    
    // Filter section
    setupFilterSection();
    
    // Preset selector
    setupPresetSelector();
    
    // Oversampling and Auto-gain controls
    setupOversamplingControls();
    setupAutoGainControls();
    
    // Phase correlation meter
    addAndMakeVisible(phaseCorrMeter);

    // Spectrum display
    addAndMakeVisible(spectrumDisplay);
    spectrumDisplay.setSpectrumAnalyzer(&audioProcessor.getSpectrumAnalyzer());
    spectrumDisplay.setParameters(&audioProcessor.getParameters());
    spectrumDisplay.setSampleRate(44100.0);  // Will be updated in timerCallback

    // Tube glow overlay (on top of everything)
    addAndMakeVisible(tubeGlow);
    tubeGlow.setOpaque(false);  // Transparent overlay
    tubeGlow.setVisible(true);
    tubeGlow.toFront(false);
    
    // Attach EQ bands to parameters
    leftChannelStrip.attachToParameters(audioProcessor.getParameters(), 0);
    rightChannelStrip.attachToParameters(audioProcessor.getParameters(), 1);
    
    // Attach hysteresis panel
    hysteresisPanel.attachToParameters(audioProcessor.getParameters());
    
    // Add parameter listener for channel linking
    audioProcessor.getParameters().state.addListener(this);
    
    // Resizing setup with locked aspect ratio
    constrainer.setMinimumSize(MIN_WIDTH, MIN_HEIGHT);
    constrainer.setMaximumSize(2000, static_cast<int>(2000 / ASPECT_RATIO));
    constrainer.setFixedAspectRatio(ASPECT_RATIO);  // Lock aspect ratio
    
    resizer = std::make_unique<juce::ResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(*resizer);
    
    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    setResizable(true, true);
    
    // Initialize base content size for zoom scaling
    baseContentWidth = DEFAULT_WIDTH;
    baseContentHeight = DEFAULT_HEIGHT;
    
    // Nebula is enabled above
    
    // Ensure all main components are in front and visible
    inputVU.toFront(false);
    outputVU.toFront(false);
    leftChannelStrip.toFront(false);
    rightChannelStrip.toFront(false);
    hysteresisPanel.toFront(false);
    
    // Start timer for UI updates
    startTimerHz(30);  // 30 Hz is sufficient
    
    // Update preset selector when program changes
    updatePresetList();
}

AetheriAudioProcessorEditor::~AetheriAudioProcessorEditor()
{
    audioProcessor.getParameters().state.removeListener(this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void AetheriAudioProcessorEditor::setupGlobalControls()
{
    // Input gain
    inputGainKnob.setValueSuffix(" dB");
    inputGainKnob.setAccentColor(Aetheri::Colors::textValue);
    inputGainKnob.setTooltip("Input Gain: Pre-EQ gain staging (±12 dB)\nDouble-click to reset");
    addAndMakeVisible(inputGainKnob);
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::inputGain, inputGainKnob.getSlider());
    
    // Output trim
    outputTrimKnob.setValueSuffix(" dB");
    outputTrimKnob.setAccentColor(Aetheri::Colors::textValue);
    outputTrimKnob.setTooltip("Output Trim: Final output level adjustment (±12 dB)\nDouble-click to reset");
    addAndMakeVisible(outputTrimKnob);
    outputTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::outputTrim, outputTrimKnob.getSlider());
    
    // Stereo mode selector
    stereoModeSelector.addItem("L/R", 1);
    stereoModeSelector.addItem("M/S", 2);
    stereoModeSelector.setTooltip("Stereo Mode: L/R (Left/Right) or M/S (Mid/Side) processing");
    addAndMakeVisible(stereoModeSelector);
    stereoModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::stereoMode, stereoModeSelector);
    
    // Channel link button
    channelLinkButton.setButtonText("LINK");
    channelLinkButton.setClickingTogglesState(true);
    channelLinkButton.setTooltip("Channel Link: Synchronize both channels (L/R mode only)");
    addAndMakeVisible(channelLinkButton);
    channelLinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::channelLink, channelLinkButton);
}

void AetheriAudioProcessorEditor::setupFilterSection()
{
    // HPF button
    hpfButton.setButtonText("HPF");
    hpfButton.setClickingTogglesState(true);
    hpfButton.setTooltip("High-Pass Filter: Remove low frequencies (12 dB/oct)");
    addAndMakeVisible(hpfButton);
    hpfButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::hpfEnabled, hpfButton);
    
    // HPF frequency knob
    hpfFreqKnob.setValueSuffix(" Hz");
    hpfFreqKnob.setAccentColor(Aetheri::Colors::bandLF);
    hpfFreqKnob.setTooltip("HPF Frequency: Cutoff frequency (20-500 Hz)\nDouble-click to reset\nShift+drag for fine adjustment");
    addAndMakeVisible(hpfFreqKnob);
    hpfFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::hpfFreq, hpfFreqKnob.getSlider());
    
    // LPF button
    lpfButton.setButtonText("LPF");
    lpfButton.setClickingTogglesState(true);
    lpfButton.setTooltip("Low-Pass Filter: Remove high frequencies (12 dB/oct)");
    addAndMakeVisible(lpfButton);
    lpfButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::lpfEnabled, lpfButton);
    
    // LPF frequency knob
    lpfFreqKnob.setValueSuffix(" Hz");
    lpfFreqKnob.setAccentColor(Aetheri::Colors::bandHF);
    lpfFreqKnob.setTooltip("LPF Frequency: Cutoff frequency (2-20 kHz)\nDouble-click to reset\nShift+drag for fine adjustment");
    addAndMakeVisible(lpfFreqKnob);
    lpfFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::lpfFreq, lpfFreqKnob.getSlider());
}

void AetheriAudioProcessorEditor::setupPresetSelector()
{
    // Preset selector combo box
    updatePresetList();
    presetSelector.setTooltip("Select a preset");
    presetSelector.onChange = [this] {
        int selected = presetSelector.getSelectedId() - 1;  // ComboBox IDs start at 1
        if (selected >= 0 && selected < audioProcessor.getNumPrograms())
        {
            // setCurrentProgram already calls loadPreset internally
            audioProcessor.setCurrentProgram(selected);
        }
    };
    addAndMakeVisible(presetSelector);
    
    // Save preset button
    savePresetButton.setButtonText("SAVE");
    savePresetButton.setTooltip("Save current settings as a new preset");
    savePresetButton.onClick = [this] {
        // Simple approach: use timestamp-based naming
        // Users can rename presets later if needed
        auto time = juce::Time::getCurrentTime();
        juce::String presetName = "Preset " + time.formatted("%H:%M:%S");
        
        audioProcessor.saveCurrentAsPreset(presetName);
        updatePresetList();
        presetSelector.setSelectedId(audioProcessor.getNumPrograms());
    };
    addAndMakeVisible(savePresetButton);
    
    // Delete preset button
    deletePresetButton.setButtonText("DEL");
    deletePresetButton.setTooltip("Delete selected user preset");
    deletePresetButton.onClick = [this] {
        int selected = presetSelector.getSelectedId() - 1;
        if (selected >= 0 && !audioProcessor.isFactoryPreset(selected))
        {
            audioProcessor.deletePreset(selected);
            updatePresetList();
            presetSelector.setSelectedId(1);
        }
    };
    addAndMakeVisible(deletePresetButton);
    
    // Rename preset button
    renamePresetButton.setButtonText("RENAME");
    renamePresetButton.setTooltip("Rename selected user preset");
    renamePresetButton.onClick = [this] {
        int selected = presetSelector.getSelectedId() - 1;
        if (selected >= 0 && !audioProcessor.isFactoryPreset(selected))
        {
            juce::String currentName = audioProcessor.getProgramName(selected);
            
            // Use a simple approach: create timestamp-based name for now
            // Full dialog implementation can be added later if needed
            auto time = juce::Time::getCurrentTime();
            juce::String newName = "Preset " + time.formatted("%H%M%S");
            audioProcessor.renamePreset(selected, newName);
            updatePresetList();
            presetSelector.setSelectedId(selected + 1);
        }
    };
    addAndMakeVisible(renamePresetButton);
    
    // Export preset button
    exportPresetButton.setButtonText("EXPORT");
    exportPresetButton.setTooltip("Export selected preset to file");
    exportPresetButton.onClick = [this] {
        int selected = presetSelector.getSelectedId() - 1;
        if (selected >= 0)
        {
            auto chooser = std::make_shared<juce::FileChooser>("Export Preset", juce::File(), "*.xml");
            auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
            chooser->launchAsync(flags, [this, selected, chooser](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result != juce::File{})
                {
                    audioProcessor.exportPreset(selected, result);
                }
            });
        }
    };
    addAndMakeVisible(exportPresetButton);
    
    // Import preset button
    importPresetButton.setButtonText("IMPORT");
    importPresetButton.setTooltip("Import preset from file");
    importPresetButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Import Preset", juce::File(), "*.xml");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result != juce::File{} && audioProcessor.importPreset(result))
            {
                updatePresetList();
                presetSelector.setSelectedId(audioProcessor.getNumPrograms());
            }
        });
    };
    addAndMakeVisible(importPresetButton);
    
    // A/B Comparison toggle
    abToggleButton.setButtonText("A");
    abToggleButton.setClickingTogglesState(false);  // Manual toggle
    abToggleButton.setTooltip("A/B Comparison: Toggle between two settings\nClick to switch between A and B states");
    abToggleButton.onClick = [this] {
        audioProcessor.toggleAB();
        updateABButtonLabel();
    };
    addAndMakeVisible(abToggleButton);
    updateABButtonLabel();  // Initialize label
}

void AetheriAudioProcessorEditor::updateABButtonLabel()
{
    // Update button to show current state
    abToggleButton.setButtonText(audioProcessor.isStateA() ? "A" : "B");
}

void AetheriAudioProcessorEditor::setupOversamplingControls()
{
    oversamplingSelector.addItem("1x", 1);
    oversamplingSelector.addItem("2x", 2);
    oversamplingSelector.addItem("4x", 3);
    oversamplingSelector.setSelectedId(1);
    oversamplingSelector.setTooltip("Oversampling: Higher rates reduce aliasing but increase CPU usage");
    addAndMakeVisible(oversamplingSelector);
    
    oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::oversampling, oversamplingSelector);
}

void AetheriAudioProcessorEditor::setupAutoGainControls()
{
    autoGainButton.setButtonText("AUTO GAIN");
    autoGainButton.setTooltip("Auto Gain Compensation: Maintains perceived loudness when adjusting EQ");
    addAndMakeVisible(autoGainButton);

    autoGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::autoGainComp, autoGainButton);

    bypassButton.setButtonText("BYPASS");
    bypassButton.setTooltip("Bypass: Pass audio through unprocessed");
    addAndMakeVisible(bypassButton);

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getParameters(), Aetheri::ParamIDs::bypass, bypassButton);
}

void AetheriAudioProcessorEditor::updatePresetList()
{
    presetSelector.clear();
    juce::StringArray presetNames = audioProcessor.getPresetNames();
    
    for (int i = 0; i < presetNames.size(); ++i)
    {
        presetSelector.addItem(presetNames[i], i + 1);  // IDs start at 1
    }
    
    // Select current preset
    presetSelector.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
}

void AetheriAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill entire window background
    g.fillAll(Aetheri::Colors::panelBackground);
    
    // Calculate zoom scale based on current size vs base size
    float scaleX = static_cast<float>(getWidth()) / baseContentWidth;
    float scaleY = static_cast<float>(getHeight()) / baseContentHeight;
    float scale = std::min(scaleX, scaleY);  // Uniform scaling
    
    // Save graphics state before transform
    g.saveState();
    
    // Calculate centered content area
    float scaledWidth = baseContentWidth * scale;
    float scaledHeight = baseContentHeight * scale;
    float offsetX = (getWidth() - scaledWidth) * 0.5f;
    float offsetY = (getHeight() - scaledHeight) * 0.5f;
    
    // Apply transform for zoom effect
    juce::AffineTransform transform = juce::AffineTransform::scale(scale)
                                        .translated(offsetX, offsetY);
    g.addTransform(transform);
    
    // Draw content at base size (will be scaled by transform)
    auto contentBounds = juce::Rectangle<int>(0, 0, baseContentWidth, baseContentHeight).toFloat();
    
    // Header bar with semi-transparent background
    auto headerArea = contentBounds.removeFromTop(50);
    
    // Dark header background
    g.setColour(Aetheri::Colors::panelBackground.withAlpha(0.95f));
    g.fillRect(headerArea);
    
    // Header border
    g.setColour(Aetheri::Colors::panelBorder);
    g.drawLine(0.0f, headerArea.getBottom(), static_cast<float>(baseContentWidth), headerArea.getBottom(), 2.0f);
    
    // Title
    g.setColour(Aetheri::Colors::textPrimary);
    g.setFont(juce::FontOptions().withHeight(24.0f).withStyle("Bold"));
    g.drawText("AETHERI", headerArea.reduced(20.0f, 0.0f), juce::Justification::centredLeft);
    
    g.setColour(Aetheri::Colors::textSecondary);
    g.setFont(juce::FontOptions().withHeight(11.0f));
    g.drawText("High-Fidelity Dual-Channel Mastering EQ", headerArea.reduced(20.0f, 0.0f).translated(140.0f, 6.0f),
               juce::Justification::centredLeft);
    
    // Restore graphics state (remove transform)
    g.restoreState();
    
    // Draw version number in bottom right corner (in screen coordinates)
    g.setColour(Aetheri::Colors::textSecondary.withAlpha(0.6f));
    g.setFont(juce::FontOptions().withHeight(10.0f * scale));
    juce::Rectangle<int> versionArea(getWidth() - static_cast<int>(80 * scale), 
                                     getHeight() - static_cast<int>(20 * scale),
                                     static_cast<int>(75 * scale), 
                                     static_cast<int>(15 * scale));
    g.drawText(PLUGIN_VERSION, versionArea, juce::Justification::bottomRight);
}

void AetheriAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    // Draw logo on top of all components, centered in header
    if (!companyLogo.isValid())
        return;
    
    // Calculate zoom scale
    float scaleX = static_cast<float>(getWidth()) / baseContentWidth;
    float scaleY = static_cast<float>(getHeight()) / baseContentHeight;
    float scale = std::min(scaleX, scaleY);
    
    // Calculate centered content area
    float scaledWidth = baseContentWidth * scale;
    float scaledHeight = baseContentHeight * scale;
    float offsetX = (getWidth() - scaledWidth) * 0.5f;
    float offsetY = (getHeight() - scaledHeight) * 0.5f;
    
    // Header area in screen coordinates
    float headerTop = offsetY;
    float headerHeight = 50.0f * scale;
    float headerWidth = scaledWidth;
    
    // Center logo horizontally in header
    float logoHeight = 30.0f * scale;
    float logoAspect = static_cast<float>(companyLogo.getWidth()) / companyLogo.getHeight();
    float logoWidth = logoHeight * logoAspect;
    
    float logoX = offsetX + (headerWidth - logoWidth) * 0.5f;
    float logoY = headerTop + (headerHeight - logoHeight) * 0.5f;
    
    juce::Rectangle<float> logoBounds(logoX, logoY, logoWidth, logoHeight);
    g.drawImage(companyLogo, logoBounds, juce::RectanglePlacement::centred);
}

void AetheriAudioProcessorEditor::resized()
{
    // Calculate zoom scale based on current size vs base size
    float scaleX = static_cast<float>(getWidth()) / baseContentWidth;
    float scaleY = static_cast<float>(getHeight()) / baseContentHeight;
    float scale = std::min(scaleX, scaleY);  // Uniform scaling
    
    // Calculate centered content area
    float scaledWidth = baseContentWidth * scale;
    float scaledHeight = baseContentHeight * scale;
    float offsetX = (getWidth() - scaledWidth) * 0.5f;
    float offsetY = (getHeight() - scaledHeight) * 0.5f;
    
    // Set bounds for all components at scaled positions
    auto contentBounds = juce::Rectangle<int>(
        static_cast<int>(offsetX), 
        static_cast<int>(offsetY),
        static_cast<int>(scaledWidth),
        static_cast<int>(scaledHeight)
    );
    
    // Nebula visualizer fills the entire window (behind everything)
    nebulaVisualizer.setBounds(getLocalBounds());
    nebulaVisualizer.toBack();
    
    // Tube glow overlay fills entire area (on top, but transparent)
    tubeGlow.setBounds(getLocalBounds());
    
    // Header area (scaled)
    auto headerArea = contentBounds.removeFromTop(static_cast<int>(50 * scale));
    
    // Preset selector in header (right side, before company name)
    auto presetArea = headerArea.removeFromRight(static_cast<int>(260 * scale));
    presetArea.reduce(static_cast<int>(5 * scale), static_cast<int>(8 * scale));
    auto presetComboArea = presetArea.removeFromLeft(static_cast<int>(140 * scale));
    presetSelector.setBounds(presetComboArea);
    presetArea.removeFromLeft(static_cast<int>(5 * scale));
    savePresetButton.setBounds(presetArea.removeFromLeft(static_cast<int>(50 * scale)));
    presetArea.removeFromLeft(static_cast<int>(5 * scale));
    abToggleButton.setBounds(presetArea.removeFromLeft(static_cast<int>(50 * scale)));
    
    // Main content area (scaled)
    contentBounds.reduce(static_cast<int>(15 * scale), static_cast<int>(10 * scale));

    // Spectrum analyzer at the top of content area
    int spectrumHeight = static_cast<int>(120 * scale);
    auto spectrumArea = contentBounds.removeFromTop(spectrumHeight);
    spectrumArea.reduce(static_cast<int>(100 * scale), 0);  // Add margins on sides
    spectrumDisplay.setBounds(spectrumArea.reduced(0, static_cast<int>(5 * scale)));

    contentBounds.removeFromTop(static_cast<int>(5 * scale));  // Spacing

    // Resizer in corner (at window coordinates, not scaled)
    resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);

    // Professional Mastering EQ Layout (inspired by AMEK):
    // [Input VU] [HPF] [IN GAIN] | [EQ LEFT] [CENTER] [EQ RIGHT] | [OUT GAIN] [LPF] [Output VU]
    
    // Scale all dimensions based on zoom
    int vuWidth = static_cast<int>(90 * scale);
    int filterWidth = static_cast<int>(70 * scale);
    int gainKnobWidth = static_cast<int>(80 * scale);
    int centerWidth = static_cast<int>(200 * scale);  // Hysteresis + stereo controls
    int spacing = static_cast<int>(60 * scale);
    int eqStripWidth = (contentBounds.getWidth() - vuWidth * 2 - filterWidth * 2 - gainKnobWidth * 2 - centerWidth - spacing) / 2;
    
    // Left side: Input VU -> HPF -> Input Gain
    auto leftSide = contentBounds.removeFromLeft(vuWidth + filterWidth + gainKnobWidth + static_cast<int>(20 * scale));
    
    auto inputVUArea = leftSide.removeFromLeft(vuWidth);
    inputVU.setBounds(inputVUArea);
    leftSide.removeFromLeft(static_cast<int>(5 * scale));
    
    auto hpfArea = leftSide.removeFromLeft(filterWidth);
    hpfButton.setBounds(hpfArea.removeFromTop(static_cast<int>(26 * scale)).reduced(static_cast<int>(3 * scale), static_cast<int>(2 * scale)));
    hpfFreqKnob.setBounds(hpfArea.reduced(static_cast<int>(2 * scale)));
    leftSide.removeFromLeft(static_cast<int>(5 * scale));
    
    inputGainKnob.setBounds(leftSide);
    
    contentBounds.removeFromLeft(static_cast<int>(10 * scale));  // Spacing before EQ
    
    // Center: EQ Left -> Hysteresis -> EQ Right
    auto centerArea = contentBounds.removeFromLeft(eqStripWidth + centerWidth + eqStripWidth);
    
    leftChannelStrip.setBounds(centerArea.removeFromLeft(eqStripWidth));
    centerArea.removeFromLeft(static_cast<int>(5 * scale));
    
    // Hysteresis panel in center
    auto hysteresisArea = centerArea.removeFromLeft(centerWidth - static_cast<int>(10 * scale));
    auto hysteresisTop = hysteresisArea.removeFromTop(static_cast<int>(hysteresisArea.getHeight() * 0.7f));
    hysteresisPanel.setBounds(hysteresisTop);
    
    // Stereo controls below hysteresis
    auto stereoArea = hysteresisArea.reduced(static_cast<int>(5 * scale), static_cast<int>(5 * scale));
    stereoModeSelector.setBounds(stereoArea.removeFromTop(static_cast<int>(28 * scale)).reduced(static_cast<int>(2 * scale)));
    channelLinkButton.setBounds(stereoArea.removeFromTop(static_cast<int>(28 * scale)).reduced(static_cast<int>(2 * scale)));
    
    // Phase correlation meter
    phaseCorrMeter.setBounds(stereoArea.removeFromTop(static_cast<int>(40 * scale)).reduced(static_cast<int>(2 * scale)));
    stereoArea.removeFromTop(static_cast<int>(3 * scale));
    
    // Oversampling, Auto-gain, and Bypass controls
    oversamplingSelector.setBounds(stereoArea.removeFromTop(static_cast<int>(24 * scale)).reduced(static_cast<int>(2 * scale)));
    autoGainButton.setBounds(stereoArea.removeFromTop(static_cast<int>(24 * scale)).reduced(static_cast<int>(2 * scale)));
    bypassButton.setBounds(stereoArea.removeFromTop(static_cast<int>(24 * scale)).reduced(static_cast<int>(2 * scale)));

    centerArea.removeFromLeft(static_cast<int>(5 * scale));
    
    rightChannelStrip.setBounds(centerArea);
    
    contentBounds.removeFromLeft(static_cast<int>(10 * scale));  // Spacing after EQ
    
    // Right side: Output Gain -> LPF -> Output VU
    auto rightSide = contentBounds.removeFromRight(vuWidth + filterWidth + gainKnobWidth + static_cast<int>(20 * scale));
    
    auto outputVUArea = rightSide.removeFromRight(vuWidth);
    outputVU.setBounds(outputVUArea);
    rightSide.removeFromRight(static_cast<int>(5 * scale));
    
    auto lpfArea = rightSide.removeFromRight(filterWidth);
    lpfButton.setBounds(lpfArea.removeFromTop(static_cast<int>(26 * scale)).reduced(static_cast<int>(3 * scale), static_cast<int>(2 * scale)));
    lpfFreqKnob.setBounds(lpfArea.reduced(static_cast<int>(2 * scale)));
    rightSide.removeFromRight(static_cast<int>(5 * scale));
    
    outputTrimKnob.setBounds(rightSide);
}

void AetheriAudioProcessorEditor::timerCallback()
{
    updateVUMeters();
    updateNebulaEnergies();
    updateTubeGlow();
    updateChannelLabels();

    // Update phase correlation meter
    phaseCorrMeter.updateCorrelation(audioProcessor.getPhaseCorrelation());

    // Note: We don't sync the preset selector here anymore.
    // The ComboBox onChange handles setting the program, and the host
    // can change programs via setCurrentProgram which the ComboBox
    // doesn't need to track in real-time (it would cause race conditions).
}

void AetheriAudioProcessorEditor::updateVUMeters()
{
    // Sync meter modes from UI to DSP only when changed (not every frame)
    static Aetheri::MeterMode lastInputMode = Aetheri::MeterMode::RMS;
    static Aetheri::MeterMode lastOutputMode = Aetheri::MeterMode::RMS;
    
    auto inputMode = inputVU.getMode();
    auto outputMode = outputVU.getMode();
    
    // Always sync modes to ensure VU and LUFS modes work properly
    if (inputMode != lastInputMode)
    {
        audioProcessor.getInputVU().setMode(inputMode);
        audioProcessor.getInputVU().reset();  // Reset to ensure clean mode switch
        lastInputMode = inputMode;
    }
    
    if (outputMode != lastOutputMode)
    {
        audioProcessor.getOutputVU().setMode(outputMode);
        audioProcessor.getOutputVU().reset();  // Reset to ensure clean mode switch
        lastOutputMode = outputMode;
    }
    
    // Update input VU meters - use const reference to avoid copying
    const auto& inVU = audioProcessor.getInputVU();
    float inLeft = inVU.getLeft().getNormalizedLevel();
    float inRight = inVU.getRight().getNormalizedLevel();
    float inLeftPeak = inVU.getLeft().getNormalizedPeak();
    float inRightPeak = inVU.getRight().getNormalizedPeak();
    
    inputVU.setLevels(inLeft, inRight);
    inputVU.setPeakLevels(inLeftPeak, inRightPeak);
    
    // Update output VU meters
    const auto& outVU = audioProcessor.getOutputVU();
    float outLeft = outVU.getLeft().getNormalizedLevel();
    float outRight = outVU.getRight().getNormalizedLevel();
    float outLeftPeak = outVU.getLeft().getNormalizedPeak();
    float outRightPeak = outVU.getRight().getNormalizedPeak();
    
    outputVU.setLevels(outLeft, outRight);
    outputVU.setPeakLevels(outLeftPeak, outRightPeak);
    
    // Pass VU meter data to nebula visualizer for OpenGL rendering
    nebulaVisualizer.setVUMeterData(inLeft, inRight, inLeftPeak, inRightPeak,
                                    outLeft, outRight, outLeftPeak, outRightPeak);
}

void AetheriAudioProcessorEditor::updateNebulaEnergies()
{
    // Update nebula visualizer with band energies (average of both channels)
    for (int band = 0; band < 4; ++band)
    {
        float energy = (audioProcessor.getBandEnergy(band, 0) + 
                       audioProcessor.getBandEnergy(band, 1)) * 0.5f;
        nebulaVisualizer.setBandEnergy(band, energy * 10.0f);  // Scale for visibility
    }
}

void AetheriAudioProcessorEditor::updateTubeGlow()
{
    float glowIntensity = audioProcessor.getHysteresisGlowIntensity();
    
    bool hystEnabled = audioProcessor.getParameters().getRawParameterValue(
        Aetheri::ParamIDs::hystEnabled)->load() > 0.5f;
    
    tubeGlow.setEnabled(hystEnabled);
    tubeGlow.setIntensity(glowIntensity);
    hysteresisPanel.setGlowIntensity(glowIntensity);
}

void AetheriAudioProcessorEditor::updateChannelLabels()
{
    // Check if we're in M/S mode
    bool isMidSide = audioProcessor.getParameters().getRawParameterValue(
        Aetheri::ParamIDs::stereoMode)->load() > 0.5f;
    
    // Check if channels are linked
    bool linked = audioProcessor.getParameters().getRawParameterValue(
        Aetheri::ParamIDs::channelLink)->load() > 0.5f;
    
    // Hide link button in M/S mode, show in L/R mode
    channelLinkButton.setVisible(!isMidSide);
    
    // If switching to M/S mode, disable linking
    if (isMidSide)
    {
        auto* linkParam = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::channelLink);
        if (linkParam && linkParam->getValue() > 0.5f)
        {
            linkParam->setValueNotifyingHost(0.0f);
        }
    }
    
    // When linked in L/R mode, gray out and disable the right channel strip
    if (!isMidSide && linked)
    {
        rightChannelStrip.setEnabled(false);
        rightChannelStrip.setAlpha(0.4f); // Visually deemphasize
    }
    else
    {
        rightChannelStrip.setEnabled(true);
        rightChannelStrip.setAlpha(1.0f);
    }
}

void AetheriAudioProcessorEditor::valueTreePropertyChanged(juce::ValueTree& /*tree*/, const juce::Identifier& property)
{
    // Ignore if we're currently linking parameters (prevent feedback loop)
    if (isLinkingParameters)
        return;

    // Ignore if a preset is being loaded (prevent interference)
    if (audioProcessor.getIsLoadingPreset())
        return;

    // Never link in M/S mode
    bool isMidSide = audioProcessor.getParameters().getRawParameterValue(
        Aetheri::ParamIDs::stereoMode)->load() > 0.5f;
    
    if (isMidSide)
        return;
    
    // Check if channels are linked
    bool linked = audioProcessor.getParameters().getRawParameterValue(
        Aetheri::ParamIDs::channelLink)->load() > 0.5f;
    
    if (!linked)
        return;
    
    // Get the property name as string
    juce::String propName = property.toString();
    
    // Check if this is a band parameter (format: band{0-3}_{param}_{channel})
    for (int band = 0; band < 4; ++band)
    {
        juce::String bandPrefix = "band" + juce::String(band) + "_";
        
        // Check for gain, trim, freq, curve, or enabled parameters
        if (propName.startsWith(bandPrefix))
        {
            // Extract parameter type and channel
            juce::String suffix = propName.substring(bandPrefix.length());
            
            // Find the parameter type (gain_, trim_, freq_, curve_, enabled_)
            juce::String paramType;
            int channel = -1;
            
            if (suffix.startsWith("gain_"))
            {
                paramType = "gain";
                channel = suffix.substring(5).getIntValue();
            }
            else if (suffix.startsWith("trim_"))
            {
                paramType = "trim";
                channel = suffix.substring(5).getIntValue();
            }
            else if (suffix.startsWith("freq_"))
            {
                paramType = "freq";
                channel = suffix.substring(5).getIntValue();
            }
            else if (suffix.startsWith("curve_"))
            {
                paramType = "curve";
                channel = suffix.substring(6).getIntValue();
            }
            else if (suffix.startsWith("enabled_"))
            {
                paramType = "enabled";
                channel = suffix.substring(8).getIntValue();
            }
            
            if (channel >= 0 && channel < 2)
            {
                // Get the value from the changed parameter
                juce::RangedAudioParameter* param = nullptr;
                if (paramType == "gain")
                    param = audioProcessor.getParameters().getParameter(
                        Aetheri::ParamIDs::bandGain(band, channel));
                else if (paramType == "trim")
                    param = audioProcessor.getParameters().getParameter(
                        Aetheri::ParamIDs::bandTrim(band, channel));
                else if (paramType == "freq")
                    param = audioProcessor.getParameters().getParameter(
                        Aetheri::ParamIDs::bandFreq(band, channel));
                else if (paramType == "curve")
                    param = audioProcessor.getParameters().getParameter(
                        Aetheri::ParamIDs::bandCurve(band, channel));
                else if (paramType == "enabled")
                    param = audioProcessor.getParameters().getParameter(
                        Aetheri::ParamIDs::bandEnabled(band, channel));
                
                if (param)
                {
                    float value = param->getValue();
                    int otherChannel = 1 - channel;  // 0 -> 1, 1 -> 0
                    
                    // Update the other channel's parameter
                    juce::String otherParamID;
                    if (paramType == "gain")
                        otherParamID = Aetheri::ParamIDs::bandGain(band, otherChannel);
                    else if (paramType == "trim")
                        otherParamID = Aetheri::ParamIDs::bandTrim(band, otherChannel);
                    else if (paramType == "freq")
                        otherParamID = Aetheri::ParamIDs::bandFreq(band, otherChannel);
                    else if (paramType == "curve")
                        otherParamID = Aetheri::ParamIDs::bandCurve(band, otherChannel);
                    else if (paramType == "enabled")
                        otherParamID = Aetheri::ParamIDs::bandEnabled(band, otherChannel);
                    
                    if (otherParamID.isNotEmpty())
                    {
                        auto* otherParam = audioProcessor.getParameters().getParameter(otherParamID);
                        if (otherParam && std::abs(otherParam->getValue() - value) > 0.001f)  // Use epsilon for float comparison
                        {
                            // Set flag to prevent feedback loop
                            isLinkingParameters = true;
                            
                            // Update the ValueTree property directly - this will trigger the slider attachment
                            // The attachment listens to ValueTree changes, so this should update the slider
                            audioProcessor.getParameters().state.setProperty(
                                juce::Identifier(otherParamID), 
                                value, 
                                nullptr);
                            
                            // Also set the parameter value to notify the host and update DSP
                            // This ensures the parameter value is in sync
                            otherParam->setValueNotifyingHost(value);
                            
                            // Clear flag
                            isLinkingParameters = false;
                            
                            // Force a visual sync to ensure the knob position updates immediately
                            // The slider attachment should update automatically, but we force it here
                            syncKnobVisual(band, otherChannel, paramType, value);
                        }
                    }
                }
            }
            break;
        }
    }
}

void AetheriAudioProcessorEditor::syncKnobVisual(int band, int channel, const juce::String& paramType, float normalizedValue)
{
    // Get the appropriate channel strip
    Aetheri::ChannelEQStrip& strip = (channel == 0) ? leftChannelStrip : rightChannelStrip;
    Aetheri::BandControl& bandControl = strip.getBand(band);
    
    // Get the parameter to convert normalized value to actual value
    juce::RangedAudioParameter* param = nullptr;
    if (paramType == "gain")
        param = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::bandGain(band, channel));
    else if (paramType == "trim")
        param = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::bandTrim(band, channel));
    else if (paramType == "freq")
        param = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::bandFreq(band, channel));
    
    if (param)
    {
        // Convert normalized value (0-1) to actual parameter value
        float actualValue = param->convertFrom0to1(normalizedValue);
        
        // Get the appropriate knob and sync its visual position
        juce::Slider* slider = nullptr;
        if (paramType == "gain")
            slider = &bandControl.getGainKnob().getSlider();
        else if (paramType == "trim")
            slider = &bandControl.getTrimKnob().getSlider();
        else if (paramType == "freq")
            slider = &bandControl.getFreqKnob().getSlider();
        
        if (slider)
        {
            // The slider attachment should update automatically when the parameter changes,
            // but we need to ensure the slider value matches the parameter value.
            // Set the value directly without sending notification to prevent feedback
            slider->setValue(actualValue, juce::dontSendNotification);
            
            // Force repaint to show the new position immediately
            slider->repaint();
            bandControl.repaint();
        }
    }
}

bool AetheriAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    // Keyboard shortcuts for common actions
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
    {
        switch (key.getKeyCode())
        {
            case 'Z':  // Undo (handled by host)
            case 'Y':  // Redo (handled by host)
                return false;  // Let host handle undo/redo
                
            case '0':  // Reset all parameters
            {
                // Reset all band parameters to defaults
                auto& params = audioProcessor.getParameters();
                for (int band = 0; band < 4; ++band)
                {
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        if (auto* gainParam = params.getParameter(Aetheri::ParamIDs::bandGain(band, ch)))
                            gainParam->setValueNotifyingHost(gainParam->getDefaultValue());
                        if (auto* trimParam = params.getParameter(Aetheri::ParamIDs::bandTrim(band, ch)))
                            trimParam->setValueNotifyingHost(trimParam->getDefaultValue());
                        if (auto* freqParam = params.getParameter(Aetheri::ParamIDs::bandFreq(band, ch)))
                            freqParam->setValueNotifyingHost(freqParam->getDefaultValue());
                    }
                }
                // Reset global controls
                if (auto* inputGain = params.getParameter(Aetheri::ParamIDs::inputGain))
                    inputGain->setValueNotifyingHost(inputGain->getDefaultValue());
                if (auto* outputTrim = params.getParameter(Aetheri::ParamIDs::outputTrim))
                    outputTrim->setValueNotifyingHost(outputTrim->getDefaultValue());
                return true;
            }
        }
    }
    
    // Single key shortcuts (no modifiers)
    int keyCode = key.getKeyCode();
    if (keyCode == ' ')  // Space = toggle hysteresis
    {
        auto* param = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::hystEnabled);
        if (param)
        {
            param->setValueNotifyingHost(1.0f - param->getValue());
            return true;
        }
    }
    
    switch (keyCode)
    {
        case 'L':  // Toggle channel link
        {
            auto* param = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::channelLink);
            if (param)
            {
                param->setValueNotifyingHost(1.0f - param->getValue());
                return true;
            }
            break;
        }
        
        case 'M':  // Toggle stereo mode (L/R <-> M/S)
        {
            auto* param = audioProcessor.getParameters().getParameter(Aetheri::ParamIDs::stereoMode);
            if (param)
            {
                float currentValue = param->getValue();
                param->setValueNotifyingHost(currentValue < 0.5f ? 1.0f : 0.0f);
                return true;
            }
            break;
        }
    }
    
    return false;  // Let other components handle the key
}
