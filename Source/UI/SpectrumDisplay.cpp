/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    SpectrumDisplay Implementation
  ==============================================================================
*/

#include "SpectrumDisplay.h"
#include "../Utils/Parameters.h"
#include <cmath>

namespace Aetheri
{
    SpectrumDisplay::SpectrumDisplay()
    {
        setOpaque(false);
        startTimerHz(30);  // 30 FPS update rate
    }

    SpectrumDisplay::~SpectrumDisplay()
    {
        stopTimer();
    }

    void SpectrumDisplay::timerCallback()
    {
        // Always repaint to keep EQ curve updated when parameters change
        // The spectrum will only update when new audio data is available
        if (spectrumAnalyzer != nullptr && spectrumAnalyzer->isNewDataAvailable())
        {
            spectrumAnalyzer->clearNewDataFlag();
        }

        // Always repaint so EQ curve responds to parameter changes
        repaint();
    }

    void SpectrumDisplay::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();

        // Background with gradient
        juce::ColourGradient bgGradient(
            Colors::panelBackground.darker(0.3f), 0.0f, 0.0f,
            Colors::panelBackground, 0.0f, bounds.getHeight(),
            false);
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border
        g.setColour(Colors::panelBorder);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        // Draw grid
        drawGrid(g);

        // Draw spectrum
        if (showSpectrum)
        {
            drawSpectrum(g);
        }

        // Draw EQ curve on top
        if (showEQCurve)
        {
            drawEQCurve(g);
        }
    }

    void SpectrumDisplay::resized()
    {
        repaint();
    }

    void SpectrumDisplay::drawGrid(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float left = bounds.getX();
        float right = bounds.getRight();
        float top = bounds.getY();
        float bottom = bounds.getBottom();

        // Frequency grid lines (vertical)
        g.setColour(Colors::panelBorder.withAlpha(0.3f));

        for (float freq : freqMarkers)
        {
            float x = frequencyToX(freq);
            if (x >= left && x <= right)
            {
                g.drawVerticalLine(static_cast<int>(x), top, bottom);
            }
        }

        // dB grid lines (horizontal)
        for (float db : dbMarkers)
        {
            float y = dbToY(db);
            if (y >= top && y <= bottom)
            {
                // 0dB line is brighter
                if (std::abs(db) < 0.1f)
                {
                    g.setColour(Colors::panelBorder.withAlpha(0.6f));
                }
                else
                {
                    g.setColour(Colors::panelBorder.withAlpha(0.3f));
                }
                g.drawHorizontalLine(static_cast<int>(y), left, right);
            }
        }

        // Frequency labels
        g.setColour(Colors::textSecondary.withAlpha(0.6f));
        g.setFont(juce::FontOptions().withHeight(9.0f));

        for (float freq : freqMarkers)
        {
            float x = frequencyToX(freq);
            if (x >= left + 10 && x <= right - 20)
            {
                juce::String label;
                if (freq >= 1000.0f)
                {
                    label = juce::String(static_cast<int>(freq / 1000.0f)) + "k";
                }
                else
                {
                    label = juce::String(static_cast<int>(freq));
                }
                g.drawText(label, static_cast<int>(x - 15), static_cast<int>(bottom - 12), 30, 10, juce::Justification::centred);
            }
        }

        // dB labels
        for (float db : dbMarkers)
        {
            float y = dbToY(db);
            if (y >= top + 5 && y <= bottom - 15)
            {
                juce::String label = juce::String(static_cast<int>(db));
                if (db > 0) label = "+" + label;
                g.drawText(label, static_cast<int>(left + 2), static_cast<int>(y - 5), 25, 10, juce::Justification::left);
            }
        }
    }

    void SpectrumDisplay::drawSpectrum(juce::Graphics& g)
    {
        if (spectrumAnalyzer == nullptr)
            return;

        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        const auto& magnitudes = spectrumAnalyzer->getAveragedMagnitudes();

        spectrumPath.clear();
        bool pathStarted = false;

        // Draw spectrum as filled gradient
        juce::Path fillPath;
        bool fillStarted = false;

        for (int i = 1; i < SpectrumAnalyzer::numBins; ++i)
        {
            float freq = spectrumAnalyzer->getLeft().getFrequencyForBin(i);

            // Skip frequencies outside our display range
            if (freq < 20.0f || freq > 20000.0f)
                continue;

            float x = frequencyToX(freq);
            float magnitude = magnitudes[i];
            float y = magnitudeToY(magnitude);

            // Clamp to bounds
            y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);

            if (!pathStarted)
            {
                spectrumPath.startNewSubPath(x, y);
                fillPath.startNewSubPath(x, bounds.getBottom());
                fillPath.lineTo(x, y);
                pathStarted = true;
                fillStarted = true;
            }
            else
            {
                spectrumPath.lineTo(x, y);
                fillPath.lineTo(x, y);
            }
        }

        if (fillStarted)
        {
            // Close fill path
            fillPath.lineTo(bounds.getRight(), bounds.getBottom());
            fillPath.closeSubPath();

            // Draw filled spectrum with gradient
            juce::ColourGradient spectrumGradient(
                Colors::bandLMF.withAlpha(0.4f), 0.0f, bounds.getY(),
                Colors::bandLF.withAlpha(0.1f), 0.0f, bounds.getBottom(),
                false);
            g.setGradientFill(spectrumGradient);
            g.fillPath(fillPath);

            // Draw spectrum line
            g.setColour(Colors::bandLMF.withAlpha(0.8f));
            g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
        }
    }

    void SpectrumDisplay::drawEQCurve(juce::Graphics& g)
    {
        if (parameters == nullptr)
            return;

        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Check if channels are linked
        bool channelsLinked = true;
        auto* linkParam = parameters->getRawParameterValue(ParamIDs::channelLink);
        if (linkParam != nullptr)
        {
            channelsLinked = linkParam->load() > 0.5f;
        }

        // Check stereo mode (L/R vs M/S)
        bool isMidSide = false;
        auto* stereoParam = parameters->getRawParameterValue(ParamIDs::stereoMode);
        if (stereoParam != nullptr)
        {
            isMidSide = stereoParam->load() > 0.5f;
        }

        // In M/S mode, channels are never linked visually
        if (isMidSide)
        {
            channelsLinked = false;
        }

        const int numPoints = 200;

        if (channelsLinked)
        {
            // Draw single curve when linked
            eqCurvePath.clear();
            bool pathStarted = false;

            for (int i = 0; i < numPoints; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
                float freq = 20.0f * std::pow(1000.0f, t);

                float x = frequencyToX(freq);
                float responseDB = calculateEQResponseForChannel(freq, 0);
                float y = dbToY(responseDB);
                y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);

                if (!pathStarted)
                {
                    eqCurvePath.startNewSubPath(x, y);
                    pathStarted = true;
                }
                else
                {
                    eqCurvePath.lineTo(x, y);
                }
            }

            if (pathStarted)
            {
                // Glow
                g.setColour(Colors::textValue.withAlpha(0.2f));
                g.strokePath(eqCurvePath, juce::PathStrokeType(4.0f));

                // Main curve
                g.setColour(Colors::textValue.withAlpha(0.9f));
                g.strokePath(eqCurvePath, juce::PathStrokeType(2.0f));
            }

            // Draw "LINKED" label
            g.setColour(Colors::textSecondary.withAlpha(0.7f));
            g.setFont(juce::FontOptions().withHeight(10.0f));
            g.drawText("LINKED", bounds.getRight() - 60, bounds.getY() + 5, 55, 12, juce::Justification::right);
        }
        else
        {
            // Draw two separate curves for L/R or M/S
            juce::Path leftPath, rightPath;
            bool leftStarted = false, rightStarted = false;

            for (int i = 0; i < numPoints; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
                float freq = 20.0f * std::pow(1000.0f, t);
                float x = frequencyToX(freq);

                // Left/Mid channel (channel 0)
                float leftDB = calculateEQResponseForChannel(freq, 0);
                float leftY = dbToY(leftDB);
                leftY = juce::jlimit(bounds.getY(), bounds.getBottom(), leftY);

                if (!leftStarted)
                {
                    leftPath.startNewSubPath(x, leftY);
                    leftStarted = true;
                }
                else
                {
                    leftPath.lineTo(x, leftY);
                }

                // Right/Side channel (channel 1)
                float rightDB = calculateEQResponseForChannel(freq, 1);
                float rightY = dbToY(rightDB);
                rightY = juce::jlimit(bounds.getY(), bounds.getBottom(), rightY);

                if (!rightStarted)
                {
                    rightPath.startNewSubPath(x, rightY);
                    rightStarted = true;
                }
                else
                {
                    rightPath.lineTo(x, rightY);
                }
            }

            // Colors for L/R or M/S
            juce::Colour leftColor = isMidSide ? juce::Colour(0xFF66CCFF) : juce::Colour(0xFF44DD88);  // Cyan for Mid, Green for Left
            juce::Colour rightColor = isMidSide ? juce::Colour(0xFFFFAA44) : juce::Colour(0xFFFF6666);  // Orange for Side, Red for Right

            // Draw Left/Mid curve
            if (leftStarted)
            {
                g.setColour(leftColor.withAlpha(0.15f));
                g.strokePath(leftPath, juce::PathStrokeType(4.0f));
                g.setColour(leftColor.withAlpha(0.85f));
                g.strokePath(leftPath, juce::PathStrokeType(2.0f));
            }

            // Draw Right/Side curve
            if (rightStarted)
            {
                g.setColour(rightColor.withAlpha(0.15f));
                g.strokePath(rightPath, juce::PathStrokeType(4.0f));
                g.setColour(rightColor.withAlpha(0.85f));
                g.strokePath(rightPath, juce::PathStrokeType(2.0f));
            }

            // Draw labels
            g.setFont(juce::FontOptions().withHeight(10.0f));

            // Left/Mid label
            g.setColour(leftColor);
            juce::String leftLabel = isMidSide ? "MID" : "LEFT";
            g.drawText(leftLabel, bounds.getRight() - 60, bounds.getY() + 5, 55, 12, juce::Justification::right);

            // Right/Side label
            g.setColour(rightColor);
            juce::String rightLabel = isMidSide ? "SIDE" : "RIGHT";
            g.drawText(rightLabel, bounds.getRight() - 60, bounds.getY() + 17, 55, 12, juce::Justification::right);
        }

        // Draw band markers
        drawBandMarkers(g, bounds);
    }

    void SpectrumDisplay::drawBandMarkers(juce::Graphics& g, const juce::Rectangle<float>& bounds)
    {
        if (parameters == nullptr)
            return;

        // Default band frequencies
        const std::array<float, 4> defaultFreqs = { 80.0f, 400.0f, 2500.0f, 8000.0f };

        for (int band = 0; band < 4; ++band)
        {
            // Get band parameters (average of both channels)
            float freq = defaultFreqs[band];
            float gainDB = 0.0f;
            bool enabled = true;

            // Try to get frequency from parameters
            auto* freqParam = parameters->getRawParameterValue(ParamIDs::bandFreq(band, 0));
            if (freqParam != nullptr)
            {
                freq = freqParam->load();
            }

            auto* gainParam = parameters->getRawParameterValue(ParamIDs::bandGain(band, 0));
            if (gainParam != nullptr)
            {
                gainDB = gainParam->load();
            }

            auto* enabledParam = parameters->getRawParameterValue(ParamIDs::bandEnabled(band, 0));
            if (enabledParam != nullptr)
            {
                enabled = enabledParam->load() > 0.5f;
            }

            if (!enabled)
                continue;

            float x = frequencyToX(freq);
            float responseDB = calculateEQResponse(freq);
            float y = dbToY(responseDB);

            // Clamp to bounds
            y = juce::jlimit(bounds.getY() + 5.0f, bounds.getBottom() - 5.0f, y);

            // Draw band marker dot
            juce::Colour bandColor = Colors::getBandColor(band);
            g.setColour(bandColor.withAlpha(0.9f));
            g.fillEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f);

            // Glow around dot
            g.setColour(bandColor.withAlpha(0.3f));
            g.fillEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f);
        }
    }

    float SpectrumDisplay::frequencyToX(float freq) const
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;

        // Logarithmic scale
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        float logFreq = std::log10(juce::jlimit(minFreq, maxFreq, freq));

        float normalized = (logFreq - logMin) / (logMax - logMin);
        return bounds.getX() + normalized * bounds.getWidth();
    }

    float SpectrumDisplay::xToFrequency(float x) const
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;

        float normalized = (x - bounds.getX()) / bounds.getWidth();
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        float logFreq = logMin + normalized * (logMax - logMin);

        return std::pow(10.0f, logFreq);
    }

    float SpectrumDisplay::dbToY(float db) const
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float minDB = -48.0f;
        float maxDB = 12.0f;

        // Linear scale for dB (inverted: high dB at top)
        float normalized = (db - minDB) / (maxDB - minDB);
        return bounds.getBottom() - normalized * bounds.getHeight();
    }

    float SpectrumDisplay::magnitudeToY(float magnitude) const
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Magnitude is already 0-1 normalized
        // Map to display range (bottom = 0, top = 1)
        float minDB = -48.0f;
        float maxDB = 12.0f;
        float rangeDB = maxDB - minDB;

        // Convert back to dB scale for consistent display
        float db = minDB + magnitude * rangeDB;
        return dbToY(db);
    }

    float SpectrumDisplay::calculateEQResponse(float frequency) const
    {
        // Average of both channels (legacy method for compatibility)
        return (calculateEQResponseForChannel(frequency, 0) + calculateEQResponseForChannel(frequency, 1)) * 0.5f;
    }

    float SpectrumDisplay::calculateEQResponseForChannel(float frequency, int channel) const
    {
        if (parameters == nullptr)
            return 0.0f;

        float totalResponse = 0.0f;

        // Default band frequencies and Q values
        const std::array<float, 4> defaultFreqs = { 80.0f, 400.0f, 2500.0f, 8000.0f };
        const std::array<float, 4> defaultQs = { 0.6f, 0.7f, 0.8f, 0.9f };

        for (int band = 0; band < 4; ++band)
        {
            float freq = defaultFreqs[band];
            float gainDB = 0.0f;
            float trimDB = 0.0f;
            float q = defaultQs[band];
            bool enabled = true;
            bool isShelf = false;
            bool isHighShelf = (band == 3);  // Band 3 (HF) is high shelf when shelf mode

            // Get parameters for the specific channel
            auto* freqParam = parameters->getRawParameterValue(ParamIDs::bandFreq(band, channel));
            if (freqParam != nullptr)
            {
                freq = freqParam->load();
            }

            auto* gainParam = parameters->getRawParameterValue(ParamIDs::bandGain(band, channel));
            if (gainParam != nullptr)
            {
                gainDB = gainParam->load();
            }

            auto* trimParam = parameters->getRawParameterValue(ParamIDs::bandTrim(band, channel));
            if (trimParam != nullptr)
            {
                trimDB = trimParam->load();
            }

            auto* enabledParam = parameters->getRawParameterValue(ParamIDs::bandEnabled(band, channel));
            if (enabledParam != nullptr)
            {
                enabled = enabledParam->load() > 0.5f;
            }

            // Check for shelf curve (only bands 0 and 3)
            if (band == 0 || band == 3)
            {
                auto* curveParam = parameters->getRawParameterValue(ParamIDs::bandCurve(band, channel));
                if (curveParam != nullptr)
                {
                    isShelf = curveParam->load() > 0.5f;
                    isHighShelf = (band == 3);
                }
            }

            if (enabled)
            {
                float bandResponse = calculateBiquadResponse(frequency, freq, gainDB + trimDB, q, isShelf, isHighShelf);
                totalResponse += bandResponse;
            }
        }

        return totalResponse;
    }

    float SpectrumDisplay::calculateBiquadResponse(float frequency, float centerFreq, float gainDB, float q, bool isShelf, bool isHighShelf) const
    {
        if (std::abs(gainDB) < 0.01f)
            return 0.0f;

        // Approximate biquad magnitude response
        float omega = frequency / centerFreq;
        float logOmega = std::log2(omega);

        if (isShelf)
        {
            // Shelf filter response approximation
            float transition = 1.0f / q;
            float x;

            if (isHighShelf)
            {
                x = logOmega * transition;
            }
            else
            {
                x = -logOmega * transition;
            }

            // Smooth transition using sigmoid-like function
            float sigmoid = 0.5f + 0.5f * std::tanh(x * 2.0f);
            return gainDB * sigmoid;
        }
        else
        {
            // Bell filter response approximation
            float bandwidth = 1.0f / q;
            float normalizedDist = logOmega / bandwidth;

            // Gaussian-like rolloff
            float response = std::exp(-normalizedDist * normalizedDist * 2.0f);
            return gainDB * response;
        }
    }
}
