# C++ Project Template

A minimal, VS Code–friendly C++ template for native and cross-compilation builds.

## Goals

- CMake-based project with `CMakePresets.json`
- Native build plus cross targets:
  - `arm-none-eabi`
  - `arm-linux-eabihf`
  - `arm64-linux`
- Explicit GCC and Clang presets for native and cross builds
- Windows MSVC build preset
- Unified cross-toolchain helper in `cmake/CMakeToolchain.cmake`
- GoogleTest unit tests
- DSP example implementation and benchmark runner
- GitLab CI starter configuration
- VS Code configuration for CMake, clangd, and debugging

## Build targets

### Native

```bash
cmake --preset native-debug
cmake --build --preset native-debug
cmake --build --preset native-debug --target test
```

### Cross compile

```bash
cmake --preset arm-linux-eabihf-release
cmake --build --preset arm-linux-eabihf-release
```

### Bare-metal Cortex-M

```bash
cmake --preset arm-none-eabi-debug
cmake --build --preset arm-none-eabi-debug
```

> Bare-metal build will produce an ELF for ARM Cortex-M, but execution requires hardware or a simulator.

## Prerequisites

This template supports both container-based CI and local toolchain installs.

- Tool requirements are defined and consumed from `tools/manifest.yaml`.
- `scripts/setup-machine.sh` and `scripts/setup-machine.ps1` parse the manifest directly, so tool requirements are documented in one place.
- A Windows helper is also available in `scripts/setup-machine.ps1`.
- On Linux, the default tool root is `/home/share/tools`.
- On Windows, the default tool root is `C:/tools`.
- Use `TOOLS_ROOT` to point to a shared path if you maintain a common tool folder.

Example local bootstrap:

```bash
./scripts/setup-machine.sh --tools-root /home/share/tools
```

Example CI bootstrap:

```bash
./scripts/setup-machine.sh --ci
```

Example manifest inspection:

```bash
cat tools/manifest.yaml
```

## Docker / CI image

A sample image build is provided in `docker/ubuntu-cpp-toolchain.Dockerfile`.
This image installs the same build tools and cross-toolchains used by the example GitLab pipeline.

## VS Code

- Use the built-in CMake Tools extension with presets
- `CMakePresets.json` contains platform presets
- `.clangd` enables project-wide clangd flags
- `.vscode/launch.json` configures native debugging

## External dependencies

This project uses CMake `FetchContent` for GoogleTest and keeps dependency logic in `cmake/Dependencies.cmake`.
