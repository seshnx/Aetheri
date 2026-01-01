/*
  ==============================================================================
    Aetheri - High-Fidelity Dual-Channel Mastering EQ
    PluginProcessor - Audio Processing Core
    
    Features:
    - 4-Band Passive Topology EQ (64-bit internal processing)
    - Analog Hysteresis Stage (Tube + Transformer saturation)
    - Dual-Channel (L/R or M/S) Processing
    - HPF/LPF with 12 dB/oct slope
    - VU Metering
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include "DSP/PassiveEQ.h"
#include "DSP/HysteresisProcessor.h"
#include "DSP/VUMeter.h"
#include "DSP/HighLowPassFilters.h"
#include "DSP/SpectrumAnalyzer.h"
#include "Utils/Parameters.h"

class AetheriAudioProcessor : public juce::AudioProcessor
{
public:
    AetheriAudioProcessor();
    ~AetheriAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    // Preset management
    void loadPreset(int index);
    void saveCurrentAsPreset(const juce::String& name);
    void deletePreset(int index);
    void renamePreset(int index, const juce::String& newName);
    bool exportPreset(int index, const juce::File& file);
    bool importPreset(const juce::File& file);
    juce::StringArray getPresetNames() const;
    bool isFactoryPreset(int index) const;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter access
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }
    
    // Metering access (for UI)
    float getInputLevel(int channel) const;
    float getOutputLevel(int channel) const;
    float getBandEnergy(int band, int channel) const;
    
    // VU Meters
    const Aetheri::StereoVUMeter& getInputVU() const { return inputVU; }
    const Aetheri::StereoVUMeter& getOutputVU() const { return outputVU; }
    Aetheri::StereoVUMeter& getInputVU() { return inputVU; }
    Aetheri::StereoVUMeter& getOutputVU() { return outputVU; }
    
    // Hysteresis glow intensity
    float getHysteresisGlowIntensity() const;
    
    // Phase correlation (for meter)
    float getPhaseCorrelation() const { return phaseCorrelation; }

    // Spectrum analyzer (for UI)
    Aetheri::StereoSpectrumAnalyzer& getSpectrumAnalyzer() { return spectrumAnalyzer; }

    // A/B Comparison methods
    void toggleAB();
    bool isStateA() const { return currentIsStateA; }

private:
    // Parameter tree state
    juce::AudioProcessorValueTreeState parameters;
    
    // DSP Processors
    Aetheri::PassiveEQ passiveEQ;
    Aetheri::HysteresisProcessor hysteresis;
    Aetheri::FilterSection filterSection;
    
    // Metering
    Aetheri::StereoVUMeter inputVU;
    Aetheri::StereoVUMeter outputVU;

    // Spectrum analyzer
    Aetheri::StereoSpectrumAnalyzer spectrumAnalyzer;
    
    // Cached parameter values
    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* outputTrimParam = nullptr;
    std::atomic<float>* stereoModeParam = nullptr;
    std::atomic<float>* channelLinkParam = nullptr;
    std::atomic<float>* hpfEnabledParam = nullptr;
    std::atomic<float>* hpfFreqParam = nullptr;
    std::atomic<float>* lpfEnabledParam = nullptr;
    std::atomic<float>* lpfFreqParam = nullptr;
    std::atomic<float>* hystEnabledParam = nullptr;
    std::atomic<float>* tubeHarmonicsParam = nullptr;
    std::atomic<float>* transformerSatParam = nullptr;
    std::atomic<float>* hystMixParam = nullptr;
    std::atomic<float>* oversamplingParam = nullptr;
    std::atomic<float>* autoGainCompParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    
    // Band parameters [band][channel]
    std::array<std::array<std::atomic<float>*, 2>, 4> bandGainParams;
    std::array<std::array<std::atomic<float>*, 2>, 4> bandTrimParams;
    std::array<std::array<std::atomic<float>*, 2>, 4> bandFreqParams;
    std::array<std::array<std::atomic<float>*, 2>, 4> bandEnabledParams;
    std::array<std::array<std::atomic<float>*, 2>, 4> bandSoloParams;
    std::array<std::array<std::atomic<float>*, 2>, 4> bandMuteParams;
    std::array<std::array<std::atomic<float>*, 2>, 2> bandCurveParams;  // Only LF and HF
    
    // Preset management
    int currentProgram = 0;
    juce::StringArray userPresetNames;
    std::vector<juce::ValueTree> userPresets;
    
    // A/B Comparison
    juce::ValueTree stateA;
    juce::ValueTree stateB;
    bool currentIsStateA = true;  // Currently viewing state A
    
    // Oversampling
    int oversamplingFactor = 1;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    
    // Auto-gain compensation
    float referenceLevel = 0.0f;  // Reference level for auto-gain
    juce::SmoothedValue<float> autoGainAdjustment;
    
    // Phase correlation
    float phaseCorrelation = 0.0f;
    
    void updateDSPFromParameters();
    void calculatePhaseCorrelation(const juce::AudioBuffer<float>& buffer);
    float calculateAutoGainAdjustment();
    
    // A/B Comparison methods (private helpers)
    void saveCurrentToAB();
    void loadABState(bool loadA);

    // Flag to prevent parameter linking during preset loading
    bool isLoadingPreset = false;
public:
    bool getIsLoadingPreset() const { return isLoadingPreset; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetheriAudioProcessor)
};
