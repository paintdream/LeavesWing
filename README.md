# LeavesWing

A cross-platform application framework built with **Qt 6** and **QML**, targeting **Windows**, **Android**, and **Linux**.

## Features

- Modern Material Design UI with a **purple** theme
- Bottom tab navigation (Home / Explore / Profile)
- Responsive layout that adapts to different screen sizes
- CMake Presets for **Visual Studio** development on Windows

## Prerequisites

| Tool | Version |
|------|---------|
| Qt | 6.5 or later |
| CMake | 3.21 or later |
| C++ Compiler | C++20 support |

### Platform-specific

- **Windows**: Visual Studio with the *Desktop development with C++* workload, using the MSVC toolchain.
- **Android**: Android NDK, Qt for Android, and the Android SDK configured in Qt Creator or via `ANDROID_NDK_ROOT` / `ANDROID_SDK_ROOT`.
- **Linux**: A C++20-capable compiler, Ninja, and a Qt 6 development package.

## Building

### Windows (Visual Studio)

The project ships with `CMakePresets.json` for Visual Studio integration.

**Option A – Open Folder in Visual Studio**

1. Open Visual Studio 2022.
2. *File → Open → Folder…* and select the repository root.
3. Visual Studio detects the CMake presets automatically.
4. Choose **windows-msvc-x64-debug** or **windows-msvc-x64-release** from the preset dropdown.
5. Build with *Build → Build All* (`Ctrl+Shift+B`).

**Option B – Command line**

```bash
cmake --preset windows-msvc-x64-debug
cmake --build --preset windows-msvc-x64-debug
```

### Android

```bash
# Set Qt and Android toolchain paths first
cmake --preset android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DQT_HOST_PATH=<path-to-host-qt>
cmake --build --preset android-arm64
```

### Linux

```bash
cmake --preset linux-x64-debug
cmake --build --preset linux-x64-debug
```

## Project Structure

```
LeavesWing/
├── CMakeLists.txt          # Build system
├── CMakePresets.json        # Visual Studio / CLI presets
├── src/
│   ├── main.cpp            # Application entry point
│   └── qml/
│       ├── Main.qml        # Root window with bottom TabBar
│       ├── HomePage.qml    # Home tab content
│       ├── ExplorePage.qml # Explore tab content
│       └── ProfilePage.qml # Profile tab content
└── platform/
    └── android/
        ├── AndroidManifest.xml
        └── res/
```

## License

MIT – see [LICENSE](LICENSE) for details.
