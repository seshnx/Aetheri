/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    SpectrumAnalyzer - FFT-based spectrum analysis
  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>

namespace Aetheri
{
    /**
     * Real-time FFT spectrum analyzer for audio visualization
     * Uses overlapping windows with Hann windowing for smooth display
     */
    class SpectrumAnalyzer
    {
    public:
        static constexpr int fftOrder = 11;  // 2048 samples
        static constexpr int fftSize = 1 << fftOrder;
        static constexpr int numBins = fftSize / 2;

        SpectrumAnalyzer();

        void prepare(double sampleRate);
        void reset();

        // Push samples into the analyzer (call from audio thread)
        void pushSamples(const float* samples, int numSamples);

        // Process FFT if enough samples collected (call from audio thread)
        void processFFT();

        // Get magnitude data for display (call from UI thread)
        // Returns data in dB, normalized 0-1 range
        const std::array<float, numBins>& getMagnitudes() const { return smoothedMagnitudes; }

        // Check if new data is available
        bool isNewDataAvailable() const { return newDataAvailable.load(); }
        void clearNewDataFlag() { newDataAvailable.store(false); }

        // Get frequency for a given bin
        float getFrequencyForBin(int bin) const;

        // Get bin for a given frequency
        int getBinForFrequency(float freq) const;

        // Get sample rate
        double getSampleRate() const { return sampleRate; }

    private:
        juce::dsp::FFT fft;
        juce::dsp::WindowingFunction<float> window;

        std::array<float, fftSize> fifo;
        std::array<float, fftSize * 2> fftData;
        std::array<float, numBins> magnitudes;
        std::array<float, numBins> smoothedMagnitudes;

        int fifoIndex = 0;
        std::atomic<bool> newDataAvailable { false };

        double sampleRate = 44100.0;

        // Smoothing factor for display (0-1, higher = smoother)
        float smoothingFactor = 0.8f;

        // Min/max dB range for normalization
        static constexpr float minDB = -90.0f;
        static constexpr float maxDB = 6.0f;
    };

    /**
     * Stereo spectrum analyzer with separate L/R analysis
     */
    class StereoSpectrumAnalyzer
    {
    public:
        StereoSpectrumAnalyzer() = default;

        void prepare(double sampleRate);
        void reset();

        // Push stereo buffer into analyzers
        void pushBuffer(const juce::AudioBuffer<float>& buffer);

        // Process FFT for both channels
        void processFFT();

        // Get analyzer for left/right channel
        const SpectrumAnalyzer& getLeft() const { return left; }
        const SpectrumAnalyzer& getRight() const { return right; }

        // Get averaged (mono) magnitudes for display
        const std::array<float, SpectrumAnalyzer::numBins>& getAveragedMagnitudes();

        bool isNewDataAvailable() const { return left.isNewDataAvailable() || right.isNewDataAvailable(); }
        void clearNewDataFlag() { left.clearNewDataFlag(); right.clearNewDataFlag(); }

    private:
        SpectrumAnalyzer left;
        SpectrumAnalyzer right;
        std::array<float, SpectrumAnalyzer::numBins> averagedMagnitudes;
    };
}
