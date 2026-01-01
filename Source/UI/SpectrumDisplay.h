/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    SpectrumDisplay - Real-time spectrum analyzer with EQ curve overlay
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/SpectrumAnalyzer.h"
#include "../DSP/PassiveEQ.h"
#include "../Utils/ColorPalette.h"

namespace Aetheri
{
    /**
     * Spectrum analyzer display with EQ curve overlay
     * Shows real-time FFT spectrum and the current EQ response curve
     */
    class SpectrumDisplay : public juce::Component,
                            public juce::Timer
    {
    public:
        SpectrumDisplay();
        ~SpectrumDisplay() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void timerCallback() override;

        // Set the spectrum analyzer reference
        void setSpectrumAnalyzer(StereoSpectrumAnalyzer* analyzer) { spectrumAnalyzer = analyzer; }

        // Set parameter tree for EQ curve calculation
        void setParameters(juce::AudioProcessorValueTreeState* params) { parameters = params; }

        // Set sample rate for frequency calculations
        void setSampleRate(double rate) { sampleRate = rate; }

        // Toggle spectrum visibility
        void setSpectrumVisible(bool visible) { showSpectrum = visible; repaint(); }
        bool isSpectrumVisible() const { return showSpectrum; }

        // Toggle EQ curve visibility
        void setEQCurveVisible(bool visible) { showEQCurve = visible; repaint(); }
        bool isEQCurveVisible() const { return showEQCurve; }

    private:
        StereoSpectrumAnalyzer* spectrumAnalyzer = nullptr;
        juce::AudioProcessorValueTreeState* parameters = nullptr;

        double sampleRate = 44100.0;
        bool showSpectrum = true;
        bool showEQCurve = true;

        // Cached path for spectrum
        juce::Path spectrumPath;

        // Cached path for EQ curve
        juce::Path eqCurvePath;

        // Frequency markers for display
        static constexpr std::array<float, 10> freqMarkers = {
            20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
        };

        // dB markers for display
        static constexpr std::array<float, 7> dbMarkers = {
            12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f
        };

        // Convert frequency to X position (logarithmic scale)
        float frequencyToX(float freq) const;

        // Convert X position to frequency
        float xToFrequency(float x) const;

        // Convert dB to Y position
        float dbToY(float db) const;

        // Convert normalized magnitude (0-1) to Y position
        float magnitudeToY(float magnitude) const;

        // Draw grid lines and labels
        void drawGrid(juce::Graphics& g);

        // Draw spectrum from analyzer
        void drawSpectrum(juce::Graphics& g);

        // Draw EQ response curve
        void drawEQCurve(juce::Graphics& g);

        // Calculate EQ magnitude response at a given frequency (averaged)
        float calculateEQResponse(float frequency) const;

        // Calculate EQ magnitude response for a specific channel
        float calculateEQResponseForChannel(float frequency, int channel) const;

        // Calculate biquad magnitude response
        float calculateBiquadResponse(float frequency, float centerFreq, float gainDB, float q, bool isShelf, bool isHighShelf) const;

        // Draw band markers on the EQ curve
        void drawBandMarkers(juce::Graphics& g, const juce::Rectangle<float>& bounds);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
    };
}
