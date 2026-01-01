# Aetheri Changelog

All notable changes to the Aetheri High-Fidelity Dual-Channel Mastering EQ plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v00.15] - 2024-12-28

### Added
- **Spectrum Analyzer**: Real-time FFT spectrum analyzer with 2048-point analysis
- **EQ Curve Overlay**: Visual representation of the current EQ response curve
- **Band Markers**: Color-coded dots showing each band's frequency and gain on the curve
- **Stereo Spectrum**: Averaged L/R spectrum display with smooth animations
- **Logarithmic Frequency Scale**: 20Hz-20kHz range with proper frequency distribution

### Changed
- Increased default window height (780px) to accommodate spectrum display
- Updated minimum window size to maintain aspect ratio

### Fixed
- **Documentation**: Fixed AAX_SETUP.md references from "Knobula" to "Aetheri"

## [v00.14] - 2024-12-24

### Added
- **Latency Reporting**: Plugin now reports oversampling latency to host for proper delay compensation
- **Bypass Parameter**: Full bypass functionality with UI toggle button
- **Multi-Platform CI**: GitHub Actions workflow builds for Windows, macOS (Universal), and Linux
- **Universal VST3 Bundle**: CI assembles cross-platform VST3 bundle with all platform binaries

### Fixed
- **Preset Loading**: Fixed race condition where timer would reset ComboBox selection before onChange fired
- **Preset Atomicity**: Added `isLoadingPreset` flag to prevent channel linking interference during preset load
- **State Sync**: Factory presets now force atomic state update via replaceState after applying

## [v00.13] - 2024-12-XX

### Added
- Multi-platform CI/CD workflow for automated builds
- Build artifacts for Windows, macOS, and Linux

## [v00.12] - 2024-12-XX

### Added
- Version number display in bottom-right corner of plugin UI
- Mac-specific build script (`build.sh`) with universal binary support
- Mac-friendly compiler configuration (Clang) in CMakeLists.txt
- Exception handling for image loading to prevent crashes

### Fixed
- Mac loading issues with FileChooser API compatibility
- Thread safety improvements for cross-platform stability
- Image loading error handling for graceful degradation

## [v00.11] - 2024-12-XX

### Added
- Auto-gain compensation feature with toggle button
- Real-time auto-gain adjustment to maintain perceived loudness when adjusting EQ
- Smooth gain compensation with 50ms adjustment smoothing

### Fixed
- Oversampling parameter updates now handled safely in prepareToPlay()
- Removed thread-unsafe prepareToPlay() calls from audio thread

## [v00.10] - 2024-12-XX

### Added
- Oversampling options (1x, 2x, 4x) with ComboBox selector
- JUCE Oversampling class integration with polyphase IIR filters
- Dynamic oversampling re-initialization when parameter changes
- Oversampling selector UI component in center panel

### Fixed
- Oversampling initialization and cleanup
- CPU usage optimization for oversampling modes

## [v00.09] - 2024-12-XX

### Added
- Phase correlation meter component for stereo phase visualization
- Real-time phase correlation calculation (-1 to +1 range)
- Color-coded meter display (green = in phase, red = out of phase)
- Phase correlation meter UI in center panel below stereo controls

### Fixed
- Phase correlation calculation accuracy
- Meter update rate optimization (30 FPS)

## [v00.08] - 2024-12-XX

### Added
- Preset management system with full CRUD operations
- Delete preset functionality (user presets only, factory presets protected)
- Rename preset functionality with timestamp-based naming
- Export preset to XML file functionality
- Import preset from XML file functionality
- Preset management buttons in header (DEL, RENAME, EXPORT, IMPORT)

### Fixed
- Preset file I/O error handling
- Preset validation and state management

## [v00.07] - 2024-12-XX

### Added
- A/B comparison feature with toggle button
- Save current state to A/B slots
- Toggle between A and B states for easy comparison
- A/B button in header showing current state (A or B)

### Fixed
- State preservation during A/B switching
- Parameter synchronization in A/B mode

## [v00.06] - 2024-12-XX

### Added
- Solo/Mute functionality per EQ band
- Solo button (S) on each band control
- Mute button (M) on each band control
- Solo logic: when any band is soloed, only soloed bands are audible
- Mute logic: muted bands are bypassed (signal passes through unchanged)
- Visual feedback: soloed bands highlighted, muted bands dimmed

### Fixed
- Band processing logic for solo/mute states
- Signal flow through muted bands

## [v00.05] - 2024-12-XX

### Added
- Mild low-end roll-off (50 Hz HPF) in hysteresis/tube stage
- Prevents distortion on high Tube simulation levels
- High-pass filter state and coefficients in HysteresisProcessor

### Fixed
- Distortion artifacts at high tube harmonics settings
- Low-frequency content handling in saturation stage

## [v00.04] - 2024-12-XX

### Added
- Factory presets system with PresetManager class
- Multiple factory presets for common mastering scenarios
- Preset selector ComboBox in header
- Save current settings as user preset functionality
- Preset loading and application system

### Fixed
- Preset initialization on plugin load
- Preset state management and persistence

## [v00.03] - 2024-12-XX

### Added
- Company logo display in plugin header
- Logo loading from binary data resources
- Centered logo positioning in header
- Logo z-order handling (drawn on top via paintOverChildren)

### Fixed
- Logo aspect ratio preservation
- Logo scaling with UI zoom

## [v00.02] - 2024-12-XX

### Added
- AAX plugin format support
- AAX SDK path configuration for Windows and macOS
- AAX build targets in CMakeLists.txt

### Fixed
- Cross-platform AAX SDK path handling

## [v00.01] - 2024-12-XX

### Added
- Initial plugin architecture and core DSP processing
- 4-band passive EQ with musical Q values
- Hysteresis processor with tube and transformer saturation
- High-pass and low-pass filter sections
- Mid/Side processing mode
- Channel linking functionality
- Input gain and output trim controls
- VU meters for input and output monitoring
- Custom analog-style UI with nebula visualizer background
- Tube glow effect visualization
- Resizable UI with locked aspect ratio
- Tooltips for all controls
- Double-click to reset parameters
- Fine/coarse adjustment mode (Shift key)
- Frequency formatting (Hz/kHz)
- Keyboard shortcuts for common operations
- Namespace organization (Aetheri namespace)
- JUCE 8.0.0 framework integration
- CMake build system configuration
- Windows build scripts (build.ps1, build.bat)

### Technical Details
- C++17 standard
- JUCE 8.0.0 framework
- CMake 3.22+ build system
- 64-bit floating-point internal processing
- Zero-latency IIR filters
- Parameter smoothing (20-50ms)
- VU meter physics-based animation

---

## Version History Summary

- **v00.15**: Spectrum analyzer with EQ curve overlay, documentation fixes
- **v00.14**: Latency reporting, bypass, preset fixes, multi-platform CI
- **v00.13**: Multi-platform CI/CD workflow
- **v00.12**: Version display, Mac build support, stability improvements
- **v00.11**: Auto-gain compensation
- **v00.10**: Oversampling options
- **v00.09**: Phase correlation meter
- **v00.08**: Preset management (delete, rename, export, import)
- **v00.07**: A/B comparison
- **v00.06**: Solo/Mute per band
- **v00.05**: Low-end roll-off for hysteresis
- **v00.04**: Factory presets system
- **v00.03**: Company logo integration
- **v00.02**: AAX format support
- **v00.01**: Initial release with core features

---

## Notes

- Version numbers increment by 0.01 for each fix/feature addition
- All versions are pre-release (0.x.x format)
- Future releases will follow semantic versioning (1.0.0+)

