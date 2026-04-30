# 🎮 Installing Raylib - Platform Guide

Raylib is a modern, easy-to-use graphics library for C++. Here's how to install it:

## macOS

### Using Homebrew (Easiest)
```bash
brew install raylib
```

### Manual Installation
```bash
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=MACOS
sudo make install PLATFORM=MACOS
```

## Linux (Ubuntu/Debian)

### Using apt
```bash
sudo apt-get update
sudo apt-get install -y libraylib-dev
```

### Build from Source
```bash
sudo apt-get install -y build-essential git
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=LINUX
sudo make install PLATFORM=LINUX
```

## Windows (MSVC)

### Using vcpkg
```bash
vcpkg install raylib:x64-windows
```

### Manual Setup
Download from: https://github.com/raysan5/raylib/releases

## Verify Installation

```bash
pkg-config --cflags --libs raylib
```

Should output something like:
```
-I/usr/include -I/usr/local/include -L/usr/lib -L/usr/local/lib -lraylib -lm
```

---

**Once installed, run the build script:**
```bash
chmod +x build_gui.sh
./build_gui.sh
```
