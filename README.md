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

This project exposes explicit GCC and Clang presets for both host and cross builds.

### Native presets

- `native-gcc-debug`
- `native-clang-debug`
- `native-gcc-release`
- `native-clang-release`

Example:

```bash
cmake --preset native-clang-debug && \
cmake --build --preset native-clang-debug && \
cmake --build --preset native-clang-debug --target test
```

```bash
cmake --preset native-gcc-debug && \
cmake --build --preset native-gcc-debug && \
cmake --build --preset native-gcc-debug --target test
```

### Cross-presets

- `arm-none-eabi-gcc-debug`
- `arm-none-eabi-clang-debug`
- `arm-linux-eabihf-gcc-release`
- `arm-linux-eabihf-clang-release`
- `arm64-linux-gcc-release`
- `arm64-linux-clang-release`

Example:

```bash
cmake --preset arm-linux-eabihf-clang-release
cmake --build --preset arm-linux-eabihf-clang-release
```

### Bare-metal Cortex-M

- `arm-none-eabi-gcc-debug`
- `arm-none-eabi-clang-debug`

```bash
cmake --preset arm-none-eabi-clang-debug
cmake --build --preset arm-none-eabi-clang-debug
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

## Shared CI helper

This repository centralizes CI execution in `scripts/ci-local.sh` so both GitLab and GitHub workflows can use the same build logic.

- GitLab CI references `scripts/ci-local.sh` from `.gitlab-ci.yml`
- GitHub Actions runs `scripts/ci-local.sh` from `.github/workflows/ci.yml`
- `scripts/setup-machine.sh --ci` installs prerequisites for CI containers

Example local CI command:

```bash
chmod +x scripts/setup-machine.sh scripts/ci-local.sh
./scripts/ci-local.sh --preset native-clang-debug --build --test --ctest
```

## Local CI reproduction

If you have Docker installed, you can reproduce the CI environment locally without changing your host machine.

1. Build the local CI image:

```bash
docker build -t cpp_template_ci:local -f docker/ubuntu-cpp-toolchain.Dockerfile .
```

2. Run the GitHub Actions-style matrix locally:

```bash
docker run --rm -v "$PWD":/workspace -w /workspace cpp_template_ci:local bash -lc \
  "chmod +x scripts/setup-machine.sh scripts/ci-local.sh && \
   ./scripts/ci-local.sh --ci --preset native-gcc-debug --build --test --ctest && \
   ./scripts/ci-local.sh --ci --preset native-clang-debug --build --test --ctest"
```

> Note: these commands run inside Docker as root, so they do not require `sudo` inside the container.

3. Run the GitLab CI-style build matrix locally:

```bash
for preset in native-gcc-release native-clang-release \
              arm-linux-eabihf-gcc-release arm-linux-eabihf-clang-release \
              arm64-linux-gcc-release arm64-linux-clang-release \
              arm-none-eabi-gcc-debug arm-none-eabi-clang-debug; do
  docker run --rm -v "$PWD":/workspace -w /workspace cpp_template_ci:local bash -lc \
    "chmod +x scripts/setup-machine.sh scripts/ci-local.sh && \
     ./scripts/ci-local.sh --ci --preset $preset --build"
  if [ $? -ne 0 ]; then
    echo "Preset failed: $preset"
    exit 1
  fi
done
```

4. Run the GitLab unit test job locally (use a valid native debug preset):

```bash
docker run --rm -v "$PWD":/workspace -w /workspace cpp_template_ci:local bash -lc \
  "chmod +x scripts/setup-machine.sh scripts/ci-local.sh && \
   ./scripts/ci-local.sh --ci --preset native-gcc-debug --build --test --ctest"
```

> Note: this runs inside Docker as root, so the container does not require `sudo`. If you instead run `./scripts/ci-local.sh --ci ...` directly on your host, that script may attempt `sudo` when not run as root.

These commands use the same `scripts/setup-machine.sh` and `scripts/ci-local.sh` helpers as CI, so they are the closest local approximation to GitHub Actions and GitLab CI.

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
