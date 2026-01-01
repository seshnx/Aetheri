/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    SpectrumAnalyzer Implementation
  ==============================================================================
*/

#include "SpectrumAnalyzer.h"
#include <cmath>

namespace Aetheri
{
    SpectrumAnalyzer::SpectrumAnalyzer()
        : fft(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        reset();
    }

    void SpectrumAnalyzer::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void SpectrumAnalyzer::reset()
    {
        fifo.fill(0.0f);
        fftData.fill(0.0f);
        magnitudes.fill(0.0f);
        smoothedMagnitudes.fill(0.0f);
        fifoIndex = 0;
        newDataAvailable.store(false);
    }

    void SpectrumAnalyzer::pushSamples(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fifo[fifoIndex] = samples[i];
            ++fifoIndex;

            if (fifoIndex >= fftSize)
            {
                // FIFO is full, trigger FFT processing
                processFFT();

                // Overlap: keep last half of samples
                std::copy(fifo.begin() + fftSize / 2, fifo.end(), fifo.begin());
                fifoIndex = fftSize / 2;
            }
        }
    }

    void SpectrumAnalyzer::processFFT()
    {
        // Copy FIFO to FFT buffer
        std::copy(fifo.begin(), fifo.end(), fftData.begin());

        // Apply window function
        window.multiplyWithWindowingTable(fftData.data(), fftSize);

        // Perform FFT
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        // Convert to magnitudes in dB and normalize
        for (int i = 0; i < numBins; ++i)
        {
            float magnitude = fftData[i];

            // Convert to dB
            float dB = (magnitude > 0.0f)
                ? juce::Decibels::gainToDecibels(magnitude / static_cast<float>(fftSize))
                : minDB;

            // Clamp to range
            dB = juce::jlimit(minDB, maxDB, dB);

            // Normalize to 0-1 range
            float normalized = (dB - minDB) / (maxDB - minDB);
            magnitudes[i] = normalized;

            // Apply smoothing for display
            smoothedMagnitudes[i] = smoothedMagnitudes[i] * smoothingFactor
                                  + normalized * (1.0f - smoothingFactor);
        }

        newDataAvailable.store(true);
    }

    float SpectrumAnalyzer::getFrequencyForBin(int bin) const
    {
        return static_cast<float>(bin) * static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    }

    int SpectrumAnalyzer::getBinForFrequency(float freq) const
    {
        int bin = static_cast<int>(freq * static_cast<float>(fftSize) / static_cast<float>(sampleRate));
        return juce::jlimit(0, numBins - 1, bin);
    }

    // StereoSpectrumAnalyzer implementation

    void StereoSpectrumAnalyzer::prepare(double sampleRate)
    {
        left.prepare(sampleRate);
        right.prepare(sampleRate);
        averagedMagnitudes.fill(0.0f);
    }

    void StereoSpectrumAnalyzer::reset()
    {
        left.reset();
        right.reset();
        averagedMagnitudes.fill(0.0f);
    }

    void StereoSpectrumAnalyzer::pushBuffer(const juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() >= 1)
        {
            left.pushSamples(buffer.getReadPointer(0), buffer.getNumSamples());
        }

        if (buffer.getNumChannels() >= 2)
        {
            right.pushSamples(buffer.getReadPointer(1), buffer.getNumSamples());
        }
        else if (buffer.getNumChannels() >= 1)
        {
            // Mono: copy left to right
            right.pushSamples(buffer.getReadPointer(0), buffer.getNumSamples());
        }
    }

    void StereoSpectrumAnalyzer::processFFT()
    {
        left.processFFT();
        right.processFFT();
    }

    const std::array<float, SpectrumAnalyzer::numBins>& StereoSpectrumAnalyzer::getAveragedMagnitudes()
    {
        const auto& leftMags = left.getMagnitudes();
        const auto& rightMags = right.getMagnitudes();

        for (int i = 0; i < SpectrumAnalyzer::numBins; ++i)
        {
            averagedMagnitudes[i] = (leftMags[i] + rightMags[i]) * 0.5f;
        }

        return averagedMagnitudes;
    }
}
