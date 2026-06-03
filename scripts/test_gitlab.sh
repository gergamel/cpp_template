#!/usr/bin/env bash
set -euo pipefail

IMAGE="cpp_template_ci:local"

for preset in native-gcc-release native-clang-release \
              arm-linux-eabihf-gcc-release arm-linux-eabihf-clang-release \
              arm64-linux-gcc-release arm64-linux-clang-release \
              arm-none-eabi-gcc-debug arm-none-eabi-clang-debug; do
  docker run --rm -v "$PWD":/workspace -w /workspace "$IMAGE" bash -lc \
    "rm -rf build/$preset && \
     ./scripts/ci-local.sh --ci --preset $preset --build"
  echo "Preset succeeded: $preset"
done

remove_build_dir "native-gcc-debug"
docker run --rm -v "$PWD":/workspace -w /workspace "$IMAGE" bash -lc \
  "rm -rf build/native-gcc-debug && \
   ./scripts/ci-local.sh --ci --preset native-gcc-debug --build --test --ctest"

echo "GitLab-style local CI reproduction completed successfully."