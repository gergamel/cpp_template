#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}/.."
PRESET=""
USE_CI=0
BUILD=0
TEST=0
CTEST=0
TARGET=""
SKIP_SETUP=0
DRY_RUN=0

show_help() {
    cat <<EOF
Usage: $0 --preset <preset> [options]

Options:
  --preset <preset>       Select a CMake preset from CMakePresets.json
  --build                 Configure and build the preset
  --test                  Build the preset test target
  --ctest                 Run ctest against the preset build directory
  --target <target>       Build a specific target using the preset
  --ci                    Install prerequisites in CI mode before running
  --skip-setup            Skip prerequisite installation/setup
  --dry-run               Print commands instead of executing them
  -h, --help              Show this help message

Examples:
  ./scripts/ci-local.sh --preset native-clang-debug --build --test
  ./scripts/ci-local.sh --ci --preset native-gcc-release --build
  ./scripts/ci-local.sh --preset native-debug --build --ctest
EOF
}

if [[ $# -eq 0 ]]; then
    show_help
    exit 1
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset)
            PRESET="${2:-}"
            shift 2
            ;;
        --build)
            BUILD=1
            shift
            ;;
        --test)
            TEST=1
            shift
            ;;
        --ctest)
            CTEST=1
            shift
            ;;
        --target)
            TARGET="${2:-}"
            shift 2
            ;;
        --ci)
            USE_CI=1
            shift
            ;;
        --skip-setup)
            SKIP_SETUP=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

if [[ -z "$PRESET" ]]; then
    echo "Error: --preset is required."
    show_help
    exit 1
fi

if [[ "$BUILD" -eq 0 && "$TEST" -eq 0 && "$CTEST" -eq 0 && -z "$TARGET" ]]; then
    BUILD=1
fi

run_cmd() {
    if [[ "$DRY_RUN" -eq 1 ]]; then
        echo "+ $*"
    else
        eval "$*"
    fi
}

if [[ "$USE_CI" -eq 1 && "$SKIP_SETUP" -eq 0 ]]; then
    echo "Running CI prerequisite setup"
    if [[ -x "${SCRIPT_DIR}/setup-machine.sh" ]]; then
        if [[ $EUID -ne 0 ]]; then
            run_cmd "sudo ${SCRIPT_DIR}/setup-machine.sh --ci"
        else
            run_cmd "${SCRIPT_DIR}/setup-machine.sh --ci"
        fi
    else
        echo "Missing setup helper: ${SCRIPT_DIR}/setup-machine.sh"
        exit 1
    fi
fi

BUILD_CMD="cmake --preset ${PRESET}"
BUILD_TARGET_CMD="cmake --build --preset ${PRESET}"
BUILD_DIR="${REPO_ROOT}/build/${PRESET}"

if [[ "$BUILD" -eq 1 ]]; then
    run_cmd "$BUILD_CMD"
    run_cmd "$BUILD_TARGET_CMD"
fi

if [[ -n "$TARGET" ]]; then
    run_cmd "$BUILD_TARGET_CMD --target ${TARGET}"
fi

if [[ "$TEST" -eq 1 ]]; then
    run_cmd "$BUILD_TARGET_CMD --target test"
fi

if [[ "$CTEST" -eq 1 ]]; then
    run_cmd "ctest --test-dir ${BUILD_DIR} --output-on-failure"
fi
