# Contributing to Sekiro Mod Manager (SMM)

Thank you for your interest in contributing to SMM! This document outlines how to build, test, and submit pull requests.

By participating, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md). Please report any security vulnerabilities privately per [SECURITY.md](SECURITY.md).

---

## 1. Prerequisites

| Tool | Recommended Version | Purpose |
| :--- | :--- | :--- |
| **Windows OS** | Windows 10 / 11 (64-bit) | Required for Win32 NTFS hard link deployment and Direct3D/QML hardware acceleration |
| **Visual Studio** | 2022 (MSVC v143) | C++17 compiler and toolchain |
| **CMake** | 3.25+ | Cross-platform build system generator |
| **Qt 6** | 6.7+ (MSVC 2022 64-bit) | Desktop framework (`QtQuick`, `Qml`, `LinguistTools`) |
| **Ninja** | 1.11+ (Optional) | Recommended fast build tool |

---

## 2. Setup & Building

Clone the repository with submodules recursively:

```bash
git clone --recurse-submodules https://github.com/RoL1n-SrP/sekiro-mods.git
cd sekiro-mods
```

### Building with CMake Presets

```bash
# Debug build (enables QML hot reload with --dev flag)
cmake --preset msvc-x64-debug
cmake --build --preset build-debug

# Release build (optimized production binary)
cmake --preset msvc-x64-release
cmake --build --preset build-release
```

### Running Tests & Verification

```bash
# Run CLI diagnostics and tests
./build/msvc-x64-debug/src/cli/smm.exe --help
./build/msvc-x64-debug/src/cli/smm.exe doctor --json

# Run QML linting (ensure Qt bin is in PATH)
qmllint.exe -I third_party/HuskarUI/src/imports -I src/gui/qml src/gui/qml/Main.qml src/gui/qml/components/*.qml src/gui/qml/views/*.qml
```

---

## 3. Pull Request Guidelines

1. **Coding Standards**:
   - First-party C++ code follows modern C++17 conventions.
   - QML UI code uses HuskarUI components (`HusWindow`, `HusButton`, `HusModal`, `HusTag`, etc.) and theme tokens from `HusTheme.Primary`.
   - Never hardcode saturated colors or absolute pixel font families; use theme design tokens.
2. **Commit Hygiene**:
   - Write clear, concise commit messages.
   - Ensure all submodules are updated and no unintended files are committed.
3. **Licensing**:
   - All contributions are submitted under the project's [MIT License](LICENSE).
