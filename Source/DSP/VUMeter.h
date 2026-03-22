/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    VUMeter - Slow-moving RMS Level Meter (VU-style ballistics)
  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>

namespace Aetheri
{
    /**
     * Meter readout modes
     */
    enum class MeterMode
    {
        RMS,      // RMS (Root Mean Square) - perceived loudness
        Peak,     // Peak - instantaneous maximum level
        VU,       // VU (Volume Unit) - classic VU ballistics with 300ms integration
        LUFS      // LUFS (Loudness Units relative to Full Scale) - broadcast standard
    };
    
    /**
     * Multi-mode level meter supporting RMS, Peak, VU, and LUFS
     */
    class VUMeter
    {
    public:
        VUMeter() = default;
        
        void prepare(double sampleRate);
        void reset();
        
        void pushSamples(const float* data, int numSamples);
        
        // Set meter mode
        void setMode(MeterMode mode) { meterMode = mode; reset(); }
        MeterMode getMode() const { return meterMode; }
        
        // Get current level in dB
        float getLevelDB() const;
        
        // Get normalized level (0.0 to 1.0)
        float getNormalizedLevel() const;
        
        // Get peak level (for peak indicator) - always peak regardless of mode
        float getPeakDB() const;
        float getNormalizedPeak() const;
        
    private:
        MeterMode meterMode = MeterMode::RMS;
        double sampleRate = 44100.0;
        
        // RMS calculation
        double rmsSum = 0.0;
        int rmsSampleCount = 0;
        int rmsWindowSize = 4410;  // ~50ms at 44.1kHz
        
        // VU ballistics (for VU mode)
        float vuLevel = 0.0f;
        float vuAttackCoeff = 0.0f;   // ~300ms rise time
        float vuReleaseCoeff = 0.0f;  // ~300ms fall time
        
        // Peak detection (for Peak mode and peak indicator)
        float peakLevel = 0.0f;
        float peakHoldTime = 0.0f;
        float peakDecayCoeff = 0.0f;
        int peakHoldSamples = 0;
        int currentPeakHold = 0;
        
        // LUFS calculation (K-weighting filter + gating)
        double lufsSum = 0.0;
        int lufsSampleCount = 0;
        int lufsWindowSize = 0;  // 400ms for LUFS
        float lufsLevel = 0.0f;
        
        // Current level (mode-dependent)
        float currentLevel = 0.0f;
        
        // Mode-specific calculations
        void updateRMS();
        void updatePeak();
        void updateVU();
        void updateLUFS();
        
        static constexpr float VU_REFERENCE = 1.0f;  // 0 dBFS = +3 VU
        static constexpr float MIN_DB = -60.0f;
    };
    
    /**
     * Stereo VU Meter pair
     */
    class StereoVUMeter
    {
    public:
        StereoVUMeter() = default;
        
        void prepare(double sampleRate);
        void reset();
        
        void pushSamples(const juce::AudioBuffer<float>& buffer);
        
        void setMode(MeterMode mode);
        MeterMode getMode() const { return meters[0].getMode(); }
        
        VUMeter& getLeft() { return meters[0]; }
        VUMeter& getRight() { return meters[1]; }
        
        const VUMeter& getLeft() const { return meters[0]; }
        const VUMeter& getRight() const { return meters[1]; }

        // Get stereo peak level (maximum of both channels)
        float getStereoPeakDB() const;
        float getNormalizedStereoPeak() const;

    private:
        std::array<VUMeter, 2> meters;
    };
}
