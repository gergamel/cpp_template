# CI Docker image

This directory contains an example Dockerfile for a reproducible build image.

## Build

```bash
docker build -t cpp-template-ci:ubuntu-24.04 -f docker/ubuntu-cpp-toolchain.Dockerfile .
```

## Use in GitLab CI

Replace the `image:` entry in `.gitlab-ci.yml` with the published image name:

```yaml
image: cpp-template-ci:ubuntu-24.04
```

The image includes:
- `cmake`
- `ninja`
- `gcc`/`g++`
- `gcc-arm-linux-gnueabihf` / `g++-arm-linux-gnueabihf`
- `gcc-aarch64-linux-gnu` / `g++-aarch64-linux-gnu`
- `gcc-arm-none-eabi` / `g++-arm-none-eabi`
- `binutils-arm-none-eabi`
- `python3`
