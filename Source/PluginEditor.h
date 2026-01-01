/*
  ==============================================================================
    Aetheri - High-Fidelity Dual-Channel Mastering EQ
    PluginEditor - Main UI with Analog Nebula Theme
    
    Features:
    - Fully resizable interface
    - Vintage analog hardware aesthetic
    - Nebula particle visualizer background
    - VU meters for Input/Output
    - Tube glow effect when Hysteresis is active
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "UI/AetheriLookAndFeel.h"
#include "UI/NebulaVisualizer.h"
#include "UI/VUMeterComponent.h"
#include "UI/BandControl.h"
#include "UI/HysteresisPanel.h"
#include "UI/TubeGlowEffect.h"
#include "UI/AnalogKnob.h"
#include "UI/PhaseCorrelationMeter.h"
#include "UI/SpectrumDisplay.h"

class AetheriAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::Timer,
                                     public juce::ValueTree::Listener
{
public:
    AetheriAudioProcessorEditor(AetheriAudioProcessor&);
    ~AetheriAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    AetheriAudioProcessor& audioProcessor;
    
    // Custom look and feel
    Aetheri::AetheriLookAndFeel lookAndFeel;
    
    // Background visualizer
    Aetheri::NebulaVisualizer nebulaVisualizer;
    
    // Tube glow overlay
    Aetheri::TubeGlowEffect tubeGlow;
    
    // VU Meters
    Aetheri::StereoVUMeterComponent inputVU;
    Aetheri::StereoVUMeterComponent outputVU;
    
    // EQ Channel Strips (L/R or M/S)
    Aetheri::ChannelEQStrip leftChannelStrip;
    Aetheri::ChannelEQStrip rightChannelStrip;
    
    // Hysteresis Panel
    Aetheri::HysteresisPanel hysteresisPanel;
    
    // Global Controls
    Aetheri::AnalogKnob inputGainKnob;
    Aetheri::AnalogKnob outputTrimKnob;
    
    // Filter Section
    juce::TextButton hpfButton;
    juce::TextButton lpfButton;
    Aetheri::AnalogKnob hpfFreqKnob;
    Aetheri::AnalogKnob lpfFreqKnob;
    
    // Stereo Mode / Link
    juce::ComboBox stereoModeSelector;
    juce::TextButton channelLinkButton;
    
    // Preset Management
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton;
    juce::TextButton deletePresetButton;
    juce::TextButton renamePresetButton;
    juce::TextButton exportPresetButton;
    juce::TextButton importPresetButton;
    
    // A/B Comparison
    juce::TextButton abToggleButton;
    
    // Phase Correlation Meter
    Aetheri::PhaseCorrelationMeter phaseCorrMeter;

    // Spectrum Analyzer Display
    Aetheri::SpectrumDisplay spectrumDisplay;
    
    // Oversampling
    juce::ComboBox oversamplingSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttachment;
    
    // Auto-gain compensation
    juce::ToggleButton autoGainButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoGainAttachment;

    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter attachments for global controls
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hpfButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lpfButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lpfFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> channelLinkAttachment;
    
    // Layout helpers
    void setupGlobalControls();
    void setupFilterSection();
    void setupPresetSelector();
    void updateChannelLabels();
    void updateVUMeters();
    void updateNebulaEnergies();
    void updateTubeGlow();
    void updatePresetList();
    void updateABButtonLabel();
    void setupOversamplingControls();
    void setupAutoGainControls();
    
    // ValueTree listener for parameter syncing
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    
    // Helper to sync knob visual position when linked
    void syncKnobVisual(int band, int channel, const juce::String& paramType, float normalizedValue);

    // Resizing constraints
    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableCornerComponent> resizer;
    
    static constexpr int DEFAULT_WIDTH = 1500;  // 25% wider (1200 * 1.25)
    static constexpr int DEFAULT_HEIGHT = 780;  // Increased for spectrum analyzer
    static constexpr int MIN_WIDTH = 1200;  // Scaled proportionally
    static constexpr int MIN_HEIGHT = 624;  // Scaled proportionally (keeps aspect ratio)
    
    // Version number - increment by 0.01 for each fix
    static constexpr const char* PLUGIN_VERSION = "v00.15";
    
    // Aspect ratio for locked resizing
    static constexpr double ASPECT_RATIO = static_cast<double>(DEFAULT_WIDTH) / DEFAULT_HEIGHT;
    
    // Base content size (for zoom scaling)
    int baseContentWidth = DEFAULT_WIDTH;
    int baseContentHeight = DEFAULT_HEIGHT;
    
    // Flag to prevent feedback loops in parameter linking
    bool isLinkingParameters = false;
    
    // Company logo
    juce::Image companyLogo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetheriAudioProcessorEditor)
};
