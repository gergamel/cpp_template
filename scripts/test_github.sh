#!/usr/bin/env bash

docker run --rm -v "$PWD":/workspace -w /workspace cpp_template_ci:local bash -lc \
  "./scripts/ci-local.sh --ci --preset native-gcc-debug --build --test --ctest && \
   ./scripts/ci-local.sh --ci --preset native-clang-debug --build --test --ctest"