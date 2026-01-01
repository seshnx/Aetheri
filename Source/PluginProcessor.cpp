/*
  ==============================================================================
    Aetheri - High-Fidelity Dual-Channel Mastering EQ
    PluginProcessor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/Presets.h"

AetheriAudioProcessor::AetheriAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("AetheriParameters"),
                 Aetheri::createParameterLayout())
{
    // Initialize factory presets on construction
    Aetheri::PresetManager::createFactoryPresets();
    
    // Cache parameter pointers for efficient access
    inputGainParam = parameters.getRawParameterValue(Aetheri::ParamIDs::inputGain);
    outputTrimParam = parameters.getRawParameterValue(Aetheri::ParamIDs::outputTrim);
    stereoModeParam = parameters.getRawParameterValue(Aetheri::ParamIDs::stereoMode);
    channelLinkParam = parameters.getRawParameterValue(Aetheri::ParamIDs::channelLink);
    
    hpfEnabledParam = parameters.getRawParameterValue(Aetheri::ParamIDs::hpfEnabled);
    hpfFreqParam = parameters.getRawParameterValue(Aetheri::ParamIDs::hpfFreq);
    lpfEnabledParam = parameters.getRawParameterValue(Aetheri::ParamIDs::lpfEnabled);
    lpfFreqParam = parameters.getRawParameterValue(Aetheri::ParamIDs::lpfFreq);
    
    hystEnabledParam = parameters.getRawParameterValue(Aetheri::ParamIDs::hystEnabled);
    tubeHarmonicsParam = parameters.getRawParameterValue(Aetheri::ParamIDs::tubeHarmonics);
    transformerSatParam = parameters.getRawParameterValue(Aetheri::ParamIDs::transformerSat);
    hystMixParam = parameters.getRawParameterValue(Aetheri::ParamIDs::hystMix);
    oversamplingParam = parameters.getRawParameterValue(Aetheri::ParamIDs::oversampling);
    autoGainCompParam = parameters.getRawParameterValue(Aetheri::ParamIDs::autoGainComp);
    bypassParam = parameters.getRawParameterValue(Aetheri::ParamIDs::bypass);
    
    // Cache band parameters
    for (int band = 0; band < 4; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            bandGainParams[band][ch] = parameters.getRawParameterValue(
                Aetheri::ParamIDs::bandGain(band, ch));
            bandTrimParams[band][ch] = parameters.getRawParameterValue(
                Aetheri::ParamIDs::bandTrim(band, ch));
            bandFreqParams[band][ch] = parameters.getRawParameterValue(
                Aetheri::ParamIDs::bandFreq(band, ch));
            bandEnabledParams[band][ch] = parameters.getRawParameterValue(
                Aetheri::ParamIDs::bandEnabled(band, ch));
            bandSoloParams[band][ch] = parameters.getRawParameterValue(
                Aetheri::ParamIDs::bandSolo(band, ch));
            bandMuteParams[band][ch] = parameters.getRawParameterValue(
                Aetheri::ParamIDs::bandMute(band, ch));
        }
    }
    
    // Curve params only for LF (band 0) and HF (band 3)
    for (int ch = 0; ch < 2; ++ch)
    {
        bandCurveParams[0][ch] = parameters.getRawParameterValue(
            Aetheri::ParamIDs::bandCurve(0, ch));
        bandCurveParams[1][ch] = parameters.getRawParameterValue(
            Aetheri::ParamIDs::bandCurve(3, ch));
    }
}

AetheriAudioProcessor::~AetheriAudioProcessor()
{
}

const juce::String AetheriAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AetheriAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AetheriAudioProcessor::producesMidi() const
{
    return false;
}

bool AetheriAudioProcessor::isMidiEffect() const
{
    return false;
}

double AetheriAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AetheriAudioProcessor::getNumPrograms()
{
    return Aetheri::PresetManager::getNumFactoryPresets() + static_cast<int>(userPresets.size());
}

int AetheriAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void AetheriAudioProcessor::setCurrentProgram(int index)
{
    if (index >= 0 && index < getNumPrograms())
    {
        currentProgram = index;
        loadPreset(index);
    }
}

const juce::String AetheriAudioProcessor::getProgramName(int index)
{
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    
    if (index < numFactory)
    {
        return Aetheri::PresetManager::getPresetName(index);
    }
    else
    {
        int userIndex = index - numFactory;
        if (userIndex >= 0 && userIndex < userPresetNames.size())
            return userPresetNames[userIndex];
    }
    
    return {};
}

void AetheriAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    
    if (index >= numFactory)
    {
        int userIndex = index - numFactory;
        if (userIndex >= 0 && userIndex < userPresetNames.size())
        {
            userPresetNames.set(userIndex, newName);
        }
    }
}

void AetheriAudioProcessor::loadPreset(int index)
{
    isLoadingPreset = true;

    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();

    if (index < numFactory)
    {
        // Apply factory preset, then force atomic state update
        Aetheri::PresetManager::applyPreset(parameters, index);

        // Force atomic state sync by copying and replacing
        // This ensures all parameter changes are applied together
        auto state = parameters.copyState();
        parameters.replaceState(state);
    }
    else
    {
        // Load user preset (already atomic via replaceState)
        int userIndex = index - numFactory;
        if (userIndex >= 0 && userIndex < static_cast<int>(userPresets.size()))
        {
            parameters.replaceState(userPresets[userIndex]);
        }
    }

    isLoadingPreset = false;

    // Notify host that state has changed
    updateHostDisplay();
}

void AetheriAudioProcessor::saveCurrentAsPreset(const juce::String& name)
{
    juce::ValueTree state = parameters.copyState();
    userPresets.push_back(state);
    userPresetNames.add(name);
}

juce::StringArray AetheriAudioProcessor::getPresetNames() const
{
    juce::StringArray names;
    
    // Add factory presets
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    for (int i = 0; i < numFactory; ++i)
    {
        names.add(Aetheri::PresetManager::getPresetName(i));
    }
    
    // Add user presets
    names.addArray(userPresetNames);
    
    return names;
}

bool AetheriAudioProcessor::isFactoryPreset(int index) const
{
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    return index < numFactory;
}

void AetheriAudioProcessor::deletePreset(int index)
{
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    
    if (index >= numFactory)
    {
        int userIndex = index - numFactory;
        if (userIndex >= 0 && userIndex < static_cast<int>(userPresets.size()))
        {
            userPresets.erase(userPresets.begin() + userIndex);
            userPresetNames.remove(userIndex);
            
            // Adjust current program if needed
            if (currentProgram == index)
            {
                currentProgram = 0;
                loadPreset(0);
            }
            else if (currentProgram > index)
            {
                currentProgram--;
            }
        }
    }
}

void AetheriAudioProcessor::renamePreset(int index, const juce::String& newName)
{
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    
    if (index >= numFactory)
    {
        int userIndex = index - numFactory;
        if (userIndex >= 0 && userIndex < userPresetNames.size())
        {
            userPresetNames.set(userIndex, newName);
        }
    }
}

bool AetheriAudioProcessor::exportPreset(int index, const juce::File& file)
{
    juce::ValueTree state;
    int numFactory = Aetheri::PresetManager::getNumFactoryPresets();
    
    if (index < numFactory)
    {
        // For factory presets, create a temporary state
        auto tempState = parameters.copyState();
        Aetheri::PresetManager::applyPreset(parameters, index);
        state = parameters.copyState();
        parameters.replaceState(tempState);  // Restore original
    }
    else
    {
        int userIndex = index - numFactory;
        if (userIndex >= 0 && userIndex < static_cast<int>(userPresets.size()))
        {
            state = userPresets[userIndex];
        }
        else
        {
            return false;
        }
    }
    
    if (!state.isValid())
        return false;
    
    // Create XML and save
    auto xml = state.createXml();
    if (xml != nullptr)
    {
        return xml->writeTo(file);
    }
    
    return false;
}

bool AetheriAudioProcessor::importPreset(const juce::File& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;
    
    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid())
        return false;
    
    // Add as user preset
    juce::String name = file.getFileNameWithoutExtension();
    userPresets.push_back(state);
    userPresetNames.add(name);
    
    return true;
}

void AetheriAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Update oversampling factor
    int osFactor = 1;  // Default to 1x
    if (oversamplingParam != nullptr)
    {
        osFactor = static_cast<int>(oversamplingParam->load()) + 1;  // 0=1x, 1=2x, 2=4x
    }
    
    if (osFactor != oversamplingFactor || oversampler == nullptr)
    {
        oversamplingFactor = osFactor;
        
        // Create oversampler with appropriate stages
        int numStages = (osFactor == 2) ? 1 : (osFactor == 4) ? 2 : 0;
        if (numStages > 0)
        {
            oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
                2, numStages, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
            oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
        }
        else
        {
            oversampler.reset();
        }
    }
    
    if (oversampler != nullptr)
    {
        oversampler->reset();
    }

    // Report latency to host (oversampling introduces latency)
    int reportedLatency = 0;
    if (oversampler != nullptr)
    {
        reportedLatency = static_cast<int>(oversampler->getLatencyInSamples());
    }
    setLatencySamples(reportedLatency);

    // Prepare all DSP processors
    // Note: Oversampling is handled by the oversampler class, which upsamples before processing
    // and downsamples after. The DSP processors work at the base rate.
    passiveEQ.prepare(sampleRate, samplesPerBlock);
    hysteresis.prepare(sampleRate, samplesPerBlock);
    filterSection.prepare(sampleRate, samplesPerBlock);
    
    // Prepare VU meters (at original rate)
    inputVU.prepare(sampleRate);
    outputVU.prepare(sampleRate);

    // Prepare spectrum analyzer
    spectrumAnalyzer.prepare(sampleRate);

    // Auto-gain compensation
    autoGainAdjustment.reset(sampleRate, 0.05);  // 50ms smoothing
    autoGainAdjustment.setCurrentAndTargetValue(0.0f);
    referenceLevel = 0.0f;
    
    // Initialize DSP processors with current parameter values
    updateDSPFromParameters();
}

void AetheriAudioProcessor::releaseResources()
{
    passiveEQ.reset();
    hysteresis.reset();
    filterSection.reset();
    inputVU.reset();
    outputVU.reset();
    spectrumAnalyzer.reset();
}

bool AetheriAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only support stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

float AetheriAudioProcessor::calculateAutoGainAdjustment()
{
    if (autoGainCompParam == nullptr || autoGainCompParam->load() < 0.5f)
        return 0.0f;  // Auto-gain disabled
    
    // Calculate current output level
    float currentLevel = (outputVU.getLeft().getNormalizedLevel() + 
                         outputVU.getRight().getNormalizedLevel()) * 0.5f;
    
    // On first measurement, set reference
    if (referenceLevel == 0.0f && currentLevel > 0.001f)
    {
        referenceLevel = currentLevel;
        return 0.0f;
    }
    
    // Calculate adjustment needed to maintain reference level
    if (referenceLevel > 0.001f)
    {
        float adjustment = referenceLevel - currentLevel;
        autoGainAdjustment.setTargetValue(adjustment);
        return autoGainAdjustment.getNextValue();
    }
    
    return 0.0f;
}

void AetheriAudioProcessor::calculatePhaseCorrelation(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
    {
        phaseCorrelation = 0.0f;
        return;
    }
    
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int numSamples = buffer.getNumSamples();
    
    float sumL = 0.0f, sumR = 0.0f, sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[i];
        float r = right[i];
        
        sumL += l;
        sumR += r;
        sumLR += l * r;
        sumL2 += l * l;
        sumR2 += r * r;
    }
    
    // Calculate correlation coefficient
    float meanL = sumL / numSamples;
    float meanR = sumR / numSamples;
    float covLR = (sumLR / numSamples) - (meanL * meanR);
    float varL = (sumL2 / numSamples) - (meanL * meanL);
    float varR = (sumR2 / numSamples) - (meanR * meanR);
    
    float denom = std::sqrt(varL * varR);
    if (denom > 0.0001f)
    {
        phaseCorrelation = covLR / denom;
    }
    else
    {
        phaseCorrelation = 0.0f;
    }
}

void AetheriAudioProcessor::updateDSPFromParameters()
{
    // Note: Oversampling changes are handled in prepareToPlay() - we don't update
    // the oversampler here to avoid thread safety issues. The oversampling factor
    // will be updated on the next prepareToPlay() call.
    // For now, just read the current value (it will be applied in prepareToPlay)
    if (oversamplingParam != nullptr)
    {
        int newOSFactor = static_cast<int>(oversamplingParam->load()) + 1;
        // Only update the factor if it changed - actual oversampler update happens in prepareToPlay
        if (newOSFactor != oversamplingFactor)
        {
            oversamplingFactor = newOSFactor;
            // Note: We can't safely re-initialize oversampler from audio thread
            // It will be updated in the next prepareToPlay() call
        }
    }
    
    // Auto-gain compensation adjustment
    float autoGainAdj = calculateAutoGainAdjustment();
    float baseInputGain = inputGainParam->load();
    float baseOutputTrim = outputTrimParam->load();
    
    // Input/Output gains (with auto-gain compensation)
    passiveEQ.setInputGain(baseInputGain + autoGainAdj);
    passiveEQ.setOutputTrim(baseOutputTrim);
    
    // Stereo mode
    bool isMidSide = stereoModeParam->load() > 0.5f;
    passiveEQ.setStereoMode(isMidSide);
    passiveEQ.setChannelLink(channelLinkParam->load() > 0.5f);
    
    // HPF/LPF
    filterSection.setHPFEnabled(hpfEnabledParam->load() > 0.5f);
    filterSection.setHPFFrequency(hpfFreqParam->load());
    filterSection.setLPFEnabled(lpfEnabledParam->load() > 0.5f);
    filterSection.setLPFFrequency(lpfFreqParam->load());
    
    // Hysteresis
    hysteresis.setEnabled(hystEnabledParam->load() > 0.5f);
    hysteresis.setTubeHarmonics(tubeHarmonicsParam->load());
    hysteresis.setTransformerSaturate(transformerSatParam->load());
    hysteresis.setMix(hystMixParam->load());
    
    // EQ Bands
    bool channelsLinked = channelLinkParam->load() > 0.5f;
    
    for (int band = 0; band < 4; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            // If channels are linked, use channel 0 values for both channels
            int sourceCh = channelsLinked ? 0 : ch;
            
            float gain = bandGainParams[band][sourceCh]->load();
            float trim = bandTrimParams[band][sourceCh]->load();
            float freq = bandFreqParams[band][sourceCh]->load();
            bool enabled = bandEnabledParams[band][sourceCh]->load() > 0.5f;
            
            Aetheri::EQBand::CurveType curve = Aetheri::EQBand::CurveType::Bell;
            
            // Get curve type for LF (band 0) and HF (band 3)
            if (band == 0)
            {
                curve = bandCurveParams[0][sourceCh]->load() > 0.5f ? 
                        Aetheri::EQBand::CurveType::Shelf : 
                        Aetheri::EQBand::CurveType::Bell;
            }
            else if (band == 3)
            {
                curve = bandCurveParams[1][sourceCh]->load() > 0.5f ? 
                        Aetheri::EQBand::CurveType::Shelf : 
                        Aetheri::EQBand::CurveType::Bell;
            }
            
            passiveEQ.setBandParameters(band, ch, freq, gain, trim, curve, enabled);
            
            // Solo/Mute
            bool solo = bandSoloParams[band][sourceCh]->load() > 0.5f;
            bool mute = bandMuteParams[band][sourceCh]->load() > 0.5f;
            passiveEQ.setBandSolo(band, ch, solo);
            passiveEQ.setBandMute(band, ch, mute);
        }
    }
}

void AetheriAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 2)
        return;

    // Check bypass state - pass audio through unprocessed
    if (bypassParam != nullptr && bypassParam->load() > 0.5f)
    {
        // Still update meters for visual feedback even when bypassed
        inputVU.pushSamples(buffer);
        outputVU.pushSamples(buffer);
        return;
    }

    // Update DSP parameters
    updateDSPFromParameters();
    
    // Measure input levels (before processing)
    inputVU.pushSamples(buffer);
    
    // Oversampling: upsample if needed
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> osBlock;
    
    if (oversamplingFactor > 1 && oversampler != nullptr)
    {
        osBlock = oversampler->processSamplesUp(block);
        
        // Create buffer from oversampled block
        juce::AudioBuffer<float> processBuffer(static_cast<int>(osBlock.getNumChannels()),
                                               static_cast<int>(osBlock.getNumSamples()));
        for (int ch = 0; ch < processBuffer.getNumChannels(); ++ch)
        {
            processBuffer.copyFrom(ch, 0, osBlock.getChannelPointer(ch), processBuffer.getNumSamples());
        }
        
        // Processing chain at oversampled rate:
        // 1. HPF/LPF Filters
        filterSection.processBlock(processBuffer);
        
        // 2. Passive EQ (includes input gain, M/S encoding, and output trim)
        passiveEQ.processBlock(processBuffer);
        
        // 3. Hysteresis Stage (parallel to EQ, applied after)
        hysteresis.processBlock(processBuffer);
        
        // Copy back to oversampled block
        for (int ch = 0; ch < processBuffer.getNumChannels(); ++ch)
        {
            osBlock.getSingleChannelBlock(ch).copyFrom(processBuffer, ch, 0, processBuffer.getNumSamples());
        }
        
        // Oversampling: downsample
        oversampler->processSamplesDown(block);
    }
    else
    {
        // Processing chain at normal rate:
        // 1. HPF/LPF Filters
        filterSection.processBlock(buffer);
        
        // 2. Passive EQ (includes input gain, M/S encoding, and output trim)
        passiveEQ.processBlock(buffer);
        
        // 3. Hysteresis Stage (parallel to EQ, applied after)
        hysteresis.processBlock(buffer);
    }
    
    // Calculate phase correlation (on output)
    calculatePhaseCorrelation(buffer);

    // Measure output levels
    outputVU.pushSamples(buffer);

    // Push to spectrum analyzer (output signal)
    spectrumAnalyzer.pushBuffer(buffer);
}

bool AetheriAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AetheriAudioProcessor::createEditor()
{
    return new AetheriAudioProcessorEditor(*this);
}

void AetheriAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AetheriAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

float AetheriAudioProcessor::getInputLevel(int channel) const
{
    return passiveEQ.getInputLevel(channel);
}

float AetheriAudioProcessor::getOutputLevel(int channel) const
{
    return passiveEQ.getOutputLevel(channel);
}

float AetheriAudioProcessor::getBandEnergy(int band, int channel) const
{
    return passiveEQ.getBandEnergy(band, channel);
}

float AetheriAudioProcessor::getHysteresisGlowIntensity() const
{
    return hysteresis.getGlowIntensity();
}

void AetheriAudioProcessor::saveCurrentToAB()
{
    // Save current state to the inactive slot
    if (currentIsStateA)
    {
        stateB = parameters.copyState();
    }
    else
    {
        stateA = parameters.copyState();
    }
}

void AetheriAudioProcessor::loadABState(bool loadA)
{
    if (loadA && stateA.isValid())
    {
        parameters.replaceState(stateA);
        currentIsStateA = true;
    }
    else if (!loadA && stateB.isValid())
    {
        parameters.replaceState(stateB);
        currentIsStateA = false;
    }
}

void AetheriAudioProcessor::toggleAB()
{
    // Save current state to the slot we're leaving
    saveCurrentToAB();
    
    // Switch to the other state
    currentIsStateA = !currentIsStateA;
    loadABState(currentIsStateA);
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AetheriAudioProcessor();
}
