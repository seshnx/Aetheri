/*
  ==============================================================================
    Aetheri - High-Fidelity Mastering EQ
    VUMeterComponent Implementation
  ==============================================================================
*/

#include "VUMeterComponent.h"

namespace Aetheri
{
    VUMeterComponent::VUMeterComponent(const juce::String& labelText)
        : label(labelText)
    {
        startTimerHz(30);  // 30 FPS is sufficient and reduces glitches
    }
    
    VUMeterComponent::~VUMeterComponent()
    {
        stopTimer();
    }
    
    void VUMeterComponent::setLevel(float normalizedLevel)
    {
        float newLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);
        if (std::abs(newLevel - targetLevel) > 0.001f)  // Only update if changed
        {
            targetLevel = newLevel;
            // Trigger immediate repaint for responsiveness
            repaint();
        }
    }
    
    void VUMeterComponent::setPeakLevel(float normalizedPeak)
    {
        float newPeak = juce::jlimit(0.0f, 1.0f, normalizedPeak);
        if (std::abs(newPeak - peakLevel) > 0.001f)  // Only update if changed
        {
            peakLevel = newPeak;
            repaint();
        }
    }
    
    void VUMeterComponent::timerCallback()
    {
        // Smooth level updates with faster response
        float oldLevel = smoothedLevel;
        float oldPeak = smoothedPeak;
        // Faster smoothing for more responsive meters (was 0.75f, now 0.5f for 2x faster)
        smoothedLevel = smoothedLevel * 0.5f + targetLevel * 0.5f;  // Faster response
        smoothedPeak = smoothedPeak * 0.7f + peakLevel * 0.3f;  // Faster peak response

        // Only repaint if something actually changed (reduces glitches)
        if (std::abs(smoothedLevel - oldLevel) > 0.001f || std::abs(smoothedPeak - oldPeak) > 0.001f)
        {
            repaint();
        }
    }
    
    void VUMeterComponent::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        
        drawMeterBackground(g, bounds);
        drawLEDSegments(g, bounds);
        drawScale(g, bounds);
        
        // Label at bottom
        if (label.isNotEmpty())
        {
            g.setColour(Colors::textSecondary);
            g.setFont(juce::Font("Arial", 9.0f, juce::Font::bold));
            g.drawText(label, bounds.removeFromBottom(12.0f), juce::Justification::centred);
        }
    }
    
    void VUMeterComponent::drawMeterBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Professional meter background
        g.setColour(Colors::vuBackground);
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Subtle inner shadow
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 3.0f, 1.0f);
        
        // Border
        g.setColour(Colors::panelBorder);
        g.drawRoundedRectangle(bounds, 3.0f, 1.5f);
    }
    
    void VUMeterComponent::drawLEDSegments(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Meter area (leave space for label and scale)
        auto meterArea = bounds.reduced(6.0f, 4.0f);
        if (label.isNotEmpty())
            meterArea.removeFromBottom(14.0f);
        meterArea.removeFromTop(8.0f);  // Space for scale
        
        float segmentHeight = meterArea.getHeight() / NUM_SEGMENTS;
        float segmentWidth = meterArea.getWidth() - 4.0f;
        float x = meterArea.getX() + 2.0f;
        float y = meterArea.getBottom();
        
        // Calculate how many segments should be lit
        int litSegments = static_cast<int>(smoothedLevel * NUM_SEGMENTS);
        int peakSegment = static_cast<int>(smoothedPeak * NUM_SEGMENTS);
        
        // Draw segments from bottom to top
        for (int i = 0; i < NUM_SEGMENTS; ++i)
        {
            float segmentY = y - (i + 1) * segmentHeight;
            bool isLit = i < litSegments;
            bool isPeak = (i == peakSegment && peakSegment > litSegments);
            
            // Determine color based on position
            float segmentPos = static_cast<float>(i) / NUM_SEGMENTS;
            juce::Colour segmentColor;
            
            if (segmentPos < GREEN_THRESHOLD)
            {
                // Green zone
                segmentColor = isLit ? Colors::vuGreen : Colors::vuGreen.withAlpha(0.15f);
            }
            else if (segmentPos < YELLOW_THRESHOLD)
            {
                // Yellow zone
                segmentColor = isLit ? Colors::vuYellow : Colors::vuYellow.withAlpha(0.15f);
            }
            else
            {
                // Red zone
                segmentColor = isLit ? Colors::vuRed : Colors::vuRed.withAlpha(0.15f);
            }
            
            // Draw segment
            if (isLit || isPeak)
            {
                // Lit segment - bright with glow
                g.setColour(segmentColor);
                g.fillRoundedRectangle(x, segmentY, segmentWidth, segmentHeight - 1.0f, 1.5f);
                
                // Highlight on top edge
                g.setColour(segmentColor.brighter(0.3f));
                g.drawLine(x, segmentY, x + segmentWidth, segmentY, 1.0f);
            }
            else
            {
                // Unlit segment - very dim
                g.setColour(segmentColor.withAlpha(0.08f));
                g.fillRoundedRectangle(x, segmentY, segmentWidth, segmentHeight - 1.0f, 1.5f);
            }
            
            // Peak indicator (small dot on the side)
            if (isPeak)
            {
                g.setColour(Colors::vuRed.brighter(0.5f));
                g.fillEllipse(x + segmentWidth - 3.0f, segmentY + segmentHeight * 0.5f - 1.5f, 3.0f, 3.0f);
            }
        }
    }
    
    void VUMeterComponent::drawScale(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        auto meterArea = bounds.reduced(6.0f, 4.0f);
        if (label.isNotEmpty())
            meterArea.removeFromBottom(14.0f);
        
        float meterHeight = meterArea.getHeight() - 8.0f;
        float scaleY = meterArea.getY() + 4.0f;
        
        // Draw scale markers on the left
        g.setFont(juce::Font("Arial", 7.0f, juce::Font::plain));
        g.setColour(Colors::textSecondary);
        
        // Scale markers vary by mode
        std::vector<std::pair<float, juce::String>> markers;
        
        switch (meterMode)
        {
            case MeterMode::RMS:
            case MeterMode::Peak:
                // Standard dB scale: 0, -10, -20, -30, -40 dB
                markers = {{1.0f, "0"}, {0.75f, "-10"}, {0.5f, "-20"}, {0.25f, "-30"}, {0.0f, "-40"}};
                break;
                
            case MeterMode::VU:
                // VU scale: +3, 0, -3, -7, -10, -20 VU
                markers = {{1.0f, "+3"}, {0.75f, "0"}, {0.6f, "-3"}, {0.4f, "-7"}, {0.25f, "-10"}, {0.0f, "-20"}};
                break;
                
            case MeterMode::LUFS:
                // LUFS scale: 0, -10, -20, -30, -40 LUFS
                markers = {{1.0f, "0"}, {0.75f, "-10"}, {0.5f, "-20"}, {0.25f, "-30"}, {0.0f, "-40"}};
                break;
        }
        
        for (auto& [pos, text] : markers)
        {
            float y = scaleY + meterHeight * (1.0f - pos);
            g.drawText(text, static_cast<int>(meterArea.getX() - 25.0f), static_cast<int>(y - 5.0f), 
                       23, 10, juce::Justification::centredRight);
            
            // Tick mark
            g.drawLine(meterArea.getX(), y, meterArea.getX() + 3.0f, y, 0.5f);
        }
        
        // Mode label at top
        g.setFont(juce::Font("Arial", 8.0f, juce::Font::bold));
        g.setColour(Colors::textPrimary);
        g.drawText(getModeLabel(), meterArea.removeFromTop(12.0f), juce::Justification::centred);
    }
    
    juce::String VUMeterComponent::getModeLabel() const
    {
        switch (meterMode)
        {
            case MeterMode::RMS: return "RMS";
            case MeterMode::Peak: return "PEAK";
            case MeterMode::VU: return "VU";
            case MeterMode::LUFS: return "LUFS";
            default: return "RMS";
        }
    }
    
    void VUMeterComponent::resized()
    {
        // Layout handled in paint
    }
    
    //==============================================================================
    // StereoVUMeterComponent Implementation
    //==============================================================================
    
    StereoVUMeterComponent::StereoVUMeterComponent(const juce::String& titleText)
        : title(titleText), leftMeter("L"), rightMeter("R")
    {
        addAndMakeVisible(leftMeter);
        addAndMakeVisible(rightMeter);
        
        // Mode selector
        modeSelector.addItem("RMS", 1);
        modeSelector.addItem("Peak", 2);
        modeSelector.addItem("VU", 3);
        modeSelector.addItem("LUFS", 4);
        modeSelector.setSelectedId(1);  // Default to RMS
        modeSelector.onChange = [this] { modeChanged(); };
        addAndMakeVisible(modeSelector);
    }
    
    void StereoVUMeterComponent::setMode(MeterMode mode)
    {
        leftMeter.setMode(mode);
        rightMeter.setMode(mode);
        
        // Update selector
        int id = 1;
        switch (mode)
        {
            case MeterMode::RMS: id = 1; break;
            case MeterMode::Peak: id = 2; break;
            case MeterMode::VU: id = 3; break;
            case MeterMode::LUFS: id = 4; break;
        }
        modeSelector.setSelectedId(id, juce::dontSendNotification);
    }
    
    void StereoVUMeterComponent::modeChanged()
    {
        MeterMode mode = MeterMode::RMS;
        switch (modeSelector.getSelectedId())
        {
            case 1: mode = MeterMode::RMS; break;
            case 2: mode = MeterMode::Peak; break;
            case 3: mode = MeterMode::VU; break;
            case 4: mode = MeterMode::LUFS; break;
        }
        setMode(mode);
        repaint();
    }
    
    void StereoVUMeterComponent::setLevels(float leftLevel, float rightLevel)
    {
        leftMeter.setLevel(leftLevel);
        rightMeter.setLevel(rightLevel);
        // Trigger repaint for immediate visual feedback
        leftMeter.repaint();
        rightMeter.repaint();
    }
    
    void StereoVUMeterComponent::setPeakLevels(float leftPeak, float rightPeak)
    {
        leftMeter.setPeakLevel(leftPeak);
        rightMeter.setPeakLevel(rightPeak);
        // Peak levels update on next timer callback, but ensure meters are visible
        leftMeter.repaint();
        rightMeter.repaint();
    }
    
    void StereoVUMeterComponent::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Title
        g.setColour(Colors::textPrimary);
        g.setFont(juce::Font("Arial", 11.0f, juce::Font::bold));
        g.drawText(title, bounds.removeFromTop(18.0f), juce::Justification::centred);
    }
    
    void StereoVUMeterComponent::resized()
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(20);  // Title space
        
        // Mode selector at bottom
        auto selectorArea = bounds.removeFromBottom(22);
        modeSelector.setBounds(selectorArea.reduced(2, 2));
        
        bounds.removeFromBottom(2);  // Spacing
        
        // Meters side by side
        int meterWidth = bounds.getWidth() / 2;
        leftMeter.setBounds(bounds.removeFromLeft(meterWidth));
        rightMeter.setBounds(bounds);
    }
}
