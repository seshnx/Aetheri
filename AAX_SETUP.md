# AAX Plugin Setup Guide

## Overview
This guide explains how to build and distribute an AAX version of Aetheri for Avid Pro Tools.

## Quick Start (Code Changes)

### Step 1: Download AAX SDK
1. Sign up at [Avid Developer Portal](https://www.avid.com/developer)
2. Request access to AAX SDK by emailing `audiosdk@avid.com`
3. Download the AAX SDK from your developer account
4. Extract to a location (e.g., `C:/AAX_SDK` on Windows or `~/AAX_SDK` on macOS)

### Step 2: Configure CMakeLists.txt
1. Open `Aetheri/CMakeLists.txt`
2. Find the AAX SDK configuration section (around line 24)
3. Uncomment and set the path:
   ```cmake
   juce_set_aax_sdk_path("C:/AAX_SDK")  # Windows
   # or
   juce_set_aax_sdk_path("~/AAX_SDK")   # macOS
   ```
4. Uncomment AAX in the FORMATS list:
   ```cmake
   FORMATS VST3 AU AAX Standalone
   ```

### Step 3: Rebuild
```bash
cd Aetheri/build
cmake ..
cmake --build . --config Release
```

The AAX plugin will be built at:
- `build/Aetheri_artefacts/Release/AAX/Aetheri.aaxplugin` (macOS)
- `build/Aetheri_artefacts/Release/AAX/Aetheri.aaxplugin` (Windows)

## Requirements for AAX Development

### 1. Development & Testing

#### Pro Tools Developer (Free)
- **Purpose**: Test unsigned AAX plugins
- **How to Get**: 
  1. Sign up at [Avid Developer](https://www.avid.com/developer)
  2. Request Pro Tools Developer Bundle activation code by emailing `devauth@avid.com`
  3. Download Pro Tools Developer from your Avid Developer account
- **Note**: Unsigned plugins ONLY work in Pro Tools Developer, not commercial Pro Tools

#### System Requirements
- **macOS**: macOS 10.15+ (Intel) or macOS 11.0+ (Apple Silicon)
- **Windows**: Windows 10+
- **Pro Tools**: Pro Tools Developer (latest version)

### 2. Distribution (Commercial Pro Tools)

#### PACE Signing Tools (Free from Avid)
- **Purpose**: Digitally sign AAX plugins for use in commercial Pro Tools
- **How to Get**:
  1. Test your plugin thoroughly in Pro Tools Developer
  2. Email `audiosdk@avid.com` with subject: **"PACE Eden Signing Tools Request"**
  3. Include in email:
     - Plugin overview/description
     - Screen recording showing plugin running in Pro Tools Developer
     - Company name
     - Admin full name
     - Telephone number
  4. PACE Anti-Piracy will contact you with signing information

#### Signing Process
- Plugins must be signed before they work in commercial Pro Tools
- Signing is free but requires approval from Avid/PACE
- Signed plugins work in both Pro Tools Developer and commercial Pro Tools

## Build Configuration

### Current Setup
The CMakeLists.txt has been configured with:
```cmake
FORMATS VST3 AU AAX Standalone
```

### JUCE Version
- **Current**: JUCE 7.0.9 (AAX support included)
- **Recommended**: JUCE 8.0+ (better AAX support and features)
- **Important**: AAX SDK must be downloaded separately from Avid and path set in CMakeLists.txt

### Platform Support
- **macOS**: ✅ AAX supported
- **Windows**: ✅ AAX supported
- **Linux**: ❌ AAX not supported (Pro Tools is macOS/Windows only)

## Testing Checklist

Before requesting signing tools, ensure:

- [ ] Plugin loads in Pro Tools Developer
- [ ] All parameters work correctly
- [ ] Audio processing functions properly
- [ ] UI displays correctly
- [ ] Presets save/load correctly
- [ ] Automation works
- [ ] No crashes or memory leaks
- [ ] Works at all sample rates (44.1, 48, 88.2, 96, 192 kHz)
- [ ] Works in both 32-bit and 64-bit Pro Tools (if applicable)

## Common Issues

### Plugin Doesn't Load
- Ensure you're using Pro Tools Developer (not commercial Pro Tools)
- Check that the plugin is in the correct AAX plugins folder
- Verify the plugin architecture matches Pro Tools (64-bit)

### Build Errors

#### "Use juce_set_aax_sdk_path to set up the AAX sdk"
- **Solution**: Download AAX SDK from Avid Developer Portal
- Set the path in CMakeLists.txt using `juce_set_aax_sdk_path()`
- Ensure the path points to the root AAX SDK directory

#### Other Build Issues
- Ensure JUCE 7.0.9+ is being used
- Check that AAX SDK path is correct and accessible
- On Windows, ensure Visual Studio 2019+ is installed
- Verify AAX SDK version matches your Pro Tools Developer version

### Signing Issues
- Must test in Pro Tools Developer first
- Must provide screen recording showing plugin working
- Approval process can take time

## File Locations

### macOS
- **Development**: `~/Library/Application Support/Avid/Audio/Plug-Ins/`
- **Distribution**: Same location for end users

### Windows
- **Development**: `C:\Program Files\Common Files\Avid\Audio\Plug-Ins\`
- **Distribution**: Same location for end users

## Additional Resources

- [Avid Developer Portal](https://www.avid.com/developer)
- [JUCE AAX Documentation](https://docs.juce.com/master/group__juce__audio__plugin__client.html)
- [Pro Tools SDK Documentation](https://developer.avid.com/)

## Notes

- AAX plugins are platform-specific (separate builds for macOS and Windows)
- AAX format is required for Pro Tools compatibility
- Unsigned plugins work ONLY in Pro Tools Developer
- Signed plugins work in both Developer and commercial Pro Tools
- Signing is free but requires approval from Avid/PACE

