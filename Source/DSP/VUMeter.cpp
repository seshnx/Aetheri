/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    VUMeter Implementation
  ==============================================================================
*/

#include "VUMeter.h"

namespace Aetheri
{
    void VUMeter::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;

        // VU meter has 300ms integration time
        // Attack and release times are both ~300ms for authentic VU behavior
        float vuTimeConstant = 0.3f;  // 300ms
        vuAttackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * vuTimeConstant));
        vuReleaseCoeff = vuAttackCoeff;  // Same for VU meters

        // RMS window size (~50ms for faster response)
        // Round to nearest multiple of 10 for better modulo behavior
        rmsWindowSize = static_cast<int>(std::round(sampleRate * 0.05 / 10.0f)) * 10;
        rmsWindowSize = std::max(rmsWindowSize, 10);  // Minimum 10 samples

        // LUFS window size (400ms for LUFS gating)
        // Round to nearest multiple of 10 for better modulo behavior
        lufsWindowSize = static_cast<int>(std::round(sampleRate * 0.4 / 10.0f)) * 10;
        lufsWindowSize = std::max(lufsWindowSize, 10);  // Minimum 10 samples

        // Peak hold time (~2 seconds, then decay)
        peakHoldSamples = static_cast<int>(sampleRate * 2.0);
        peakDecayCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.5f));  // 500ms decay

        reset();
    }
    
    void VUMeter::reset()
    {
        rmsSum = 0.0;
        rmsSampleCount = 0;
        vuLevel = 0.0f;
        peakLevel = 0.0f;
        peakHoldTime = 0.0f;
        currentPeakHold = 0;
        currentLevel = 0.0f;
        lufsSum = 0.0;
        lufsSampleCount = 0;
        lufsLevel = 0.0f;
    }
    
    void VUMeter::pushSamples(const float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = data[i];
            float absSample = std::abs(sample);
            
            // Always accumulate for RMS/LUFS (needed for multiple modes)
            rmsSum += static_cast<double>(sample * sample);
            rmsSampleCount++;
            
            // LUFS accumulation (same as RMS for now, K-weighting would be added here)
            lufsSum += static_cast<double>(sample * sample);
            lufsSampleCount++;
            
            // Peak detection (always active for peak indicator)
            if (absSample > peakLevel)
            {
                peakLevel = absSample;
                currentPeakHold = 0;
            }
            else
            {
                currentPeakHold++;
                if (currentPeakHold > peakHoldSamples)
                {
                    // Decay peak after hold time
                    peakLevel *= peakDecayCoeff;
                }
            }
        }
        
        // Update based on current mode (after accumulating samples)
        switch (meterMode)
        {
            case MeterMode::RMS:
                updateRMS();
                break;
            case MeterMode::Peak:
                updatePeak();
                break;
            case MeterMode::VU:
                updateVU();
                break;
            case MeterMode::LUFS:
                updateLUFS();
                break;
        }
    }
    
    void VUMeter::updateRMS()
    {
        // Calculate RMS when window is full, or update more frequently for responsiveness
        if (rmsSampleCount >= rmsWindowSize)
        {
            float rmsValue = std::sqrt(static_cast<float>(rmsSum / rmsSampleCount));
            currentLevel = rmsValue;
            
            // Reset RMS accumulator
            rmsSum = 0.0;
            rmsSampleCount = 0;
        }
        // Also update on partial windows for faster response (every 10% of window)
        else if (rmsSampleCount > 0 && rmsWindowSize >= 10 && (rmsSampleCount % (rmsWindowSize / 10)) == 0)
        {
            float rmsValue = std::sqrt(static_cast<float>(rmsSum / rmsSampleCount));
            currentLevel = currentLevel * 0.95f + rmsValue * 0.05f;  // Quick smoothing
        }
    }
    
    void VUMeter::updatePeak()
    {
        // Peak mode uses instantaneous peak with fast attack, slow release
        float instantPeak = peakLevel;
        
        if (instantPeak > currentLevel)
        {
            // Fast attack (10ms)
            float attackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.01f));
            currentLevel = currentLevel * attackCoeff + instantPeak * (1.0f - attackCoeff);
        }
        else
        {
            // Slow release (1 second)
            float releaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 1.0f));
            currentLevel = currentLevel * releaseCoeff + instantPeak * (1.0f - releaseCoeff);
        }
    }
    
    void VUMeter::updateVU()
    {
        // Calculate RMS when window is full
        if (rmsSampleCount >= rmsWindowSize)
        {
            float rmsValue = std::sqrt(static_cast<float>(rmsSum / rmsSampleCount));
            
            // Apply VU ballistics (300ms attack/release)
            if (rmsValue > vuLevel)
            {
                // Attack (rising)
                vuLevel = vuLevel * vuAttackCoeff + rmsValue * (1.0f - vuAttackCoeff);
            }
            else
            {
                // Release (falling)
                vuLevel = vuLevel * vuReleaseCoeff + rmsValue * (1.0f - vuReleaseCoeff);
            }
            
            currentLevel = vuLevel;
            
            // Reset RMS accumulator
            rmsSum = 0.0;
            rmsSampleCount = 0;
        }
        // Also update on partial windows for faster response (every 5% of window)
        else if (rmsSampleCount > 0 && rmsWindowSize >= 20 && (rmsSampleCount % (rmsWindowSize / 20)) == 0)
        {
            float rmsValue = std::sqrt(static_cast<float>(rmsSum / rmsSampleCount));
            // Quick update with VU ballistics (faster intermediate updates)
            if (rmsValue > vuLevel)
                vuLevel = vuLevel * 0.9f + rmsValue * 0.1f;  // Faster attack
            else
                vuLevel = vuLevel * 0.95f + rmsValue * 0.05f;  // Faster release
            currentLevel = vuLevel;
        }
    }
    
    void VUMeter::updateLUFS()
    {
        // LUFS uses 400ms gating window
        if (lufsSampleCount >= lufsWindowSize)
        {
            float rmsValue = std::sqrt(static_cast<float>(lufsSum / lufsSampleCount));
            
            // Simple LUFS approximation (full implementation would include K-weighting filter)
            // LUFS is typically -23 LUFS = 0 dBFS reference
            // For now, we'll use RMS with LUFS scaling
            float lufsValue = rmsValue * 0.7079f;  // Approximate K-weighting effect
            
            // Smooth LUFS level (faster response)
            lufsLevel = lufsLevel * 0.7f + lufsValue * 0.3f;
            currentLevel = lufsLevel;
            
            // Reset accumulator
            lufsSum = 0.0;
            lufsSampleCount = 0;
        }
        // Also update on partial windows for faster response (every 10% of window)
        else if (lufsSampleCount > 0 && lufsWindowSize >= 10 && (lufsSampleCount % (lufsWindowSize / 10)) == 0)
        {
            float rmsValue = std::sqrt(static_cast<float>(lufsSum / lufsSampleCount));
            float lufsValue = rmsValue * 0.7079f;
            // Quick update
            lufsLevel = lufsLevel * 0.8f + lufsValue * 0.2f;
            currentLevel = lufsLevel;
        }
    }
    
    float VUMeter::getLevelDB() const
    {
        float level = currentLevel;
        
        // Use a higher threshold to avoid log(0) issues
        if (level < 1e-8f)
            return MIN_DB;
        
        float db = 0.0f;
        
        switch (meterMode)
        {
            case MeterMode::RMS:
                // RMS: 0 dBFS = 0 dB
                db = 20.0f * std::log10(level);
                break;
                
            case MeterMode::Peak:
                // Peak: 0 dBFS = 0 dB
                db = 20.0f * std::log10(level);
                break;
                
            case MeterMode::VU:
                // VU: 0 VU = -3 dBFS, so add 3 dB offset
                db = 20.0f * std::log10(level) + 3.0f;
                break;
                
            case MeterMode::LUFS:
                // LUFS: Approximate EBU R128 standard
                // 0 dBFS sine wave ≈ -3 LUFS (with K-weighting)
                // Simplified: use -3 dB offset to approximate K-weighting effect
                db = 20.0f * std::log10(level) - 3.0f;
                break;
        }
        
        return std::max(MIN_DB, db);
    }
    
    float VUMeter::getNormalizedLevel() const
    {
        float db = getLevelDB();
        
        // Normalize based on meter mode
        switch (meterMode)
        {
            case MeterMode::RMS:
            case MeterMode::Peak:
                // Standard dB scale: -60 to 0 dB maps to 0.0 to 1.0
                if (db < -60.0f)
                    return 0.0f;
                return (db + 60.0f) / 60.0f;
                
            case MeterMode::VU:
                // VU scale: -20 to +3 VU maps to 0.0 to 1.0
                // 0 VU = -3 dBFS maps to 0.75
                if (db < -20.0f)
                    return 0.0f;
                if (db < -3.0f)
                {
                    // -20 to -3 dBFS maps to 0.0 to 0.75
                    // At -3 dBFS (0 VU), we want 0.75
                    return (db + 20.0f) / 17.0f * 0.75f;
                }
                else
                {
                    // -3 to 0 dBFS maps to 0.75 to 1.0 (+3 VU)
                    return 0.75f + ((db + 3.0f) / 3.0f) * 0.25f;
                }
                
            case MeterMode::LUFS:
                // LUFS scale: -40 to 0 LUFS maps to 0.0 to 1.0
                // -3 LUFS (0 dBFS) should be near top, -40 LUFS at bottom
                if (db < -40.0f)
                    return 0.0f;
                return (db + 40.0f) / 40.0f;  // -40 to 0 = 40 dB range
                
            default:
                return 0.0f;
        }
    }
    
    float VUMeter::getPeakDB() const
    {
        if (peakLevel < 1e-10f)
            return MIN_DB;
        
        return 20.0f * std::log10(peakLevel);
    }
    
    float VUMeter::getNormalizedPeak() const
    {
        float db = getPeakDB();
        
        // Map -40 dB to 0.0, 0 dBFS to 1.0
        if (db < -40.0f)
            return 0.0f;
        
        return (db + 40.0f) / 40.0f;
    }
    
    //==============================================================================
    // StereoVUMeter Implementation
    //==============================================================================
    
    void StereoVUMeter::prepare(double sampleRate)
    {
        for (auto& meter : meters)
        {
            meter.prepare(sampleRate);
        }
    }
    
    void StereoVUMeter::reset()
    {
        for (auto& meter : meters)
        {
            meter.reset();
        }
    }
    
    void StereoVUMeter::pushSamples(const juce::AudioBuffer<float>& buffer)
    {
        int numChannels = std::min(buffer.getNumChannels(), 2);
        int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch)
        {
            meters[ch].pushSamples(buffer.getReadPointer(ch), numSamples);
        }
    }
    
    void StereoVUMeter::setMode(MeterMode mode)
    {
        for (auto& meter : meters)
        {
            meter.setMode(mode);
        }
    }

    float StereoVUMeter::getStereoPeakDB() const
    {
        float leftPeak = meters[0].getPeakDB();
        float rightPeak = meters[1].getPeakDB();
        return std::max(leftPeak, rightPeak);
    }

    float StereoVUMeter::getNormalizedStereoPeak() const
    {
        float leftPeak = meters[0].getNormalizedPeak();
        float rightPeak = meters[1].getNormalizedPeak();
        return std::max(leftPeak, rightPeak);
    }
}
