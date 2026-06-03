#!/usr/bin/env bash
set -euo pipefail

TOOLS_ROOT=""
CI_MODE=0
DRY_RUN=0
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST_FILE="${SCRIPT_DIR}/../tools/manifest.yaml"
MANIFEST_HELPER="${SCRIPT_DIR}/manifest-helper.py"

default_tools_root() {
    if [[ "$OSTYPE" == cygwin* || "$OSTYPE" == msys* || "$OSTYPE" == win32* ]]; then
        printf "%s" "C:/tools"
    else
        printf "%s" "/home/share/tools"
    fi
}

show_help() {
    cat <<EOF
Usage: $0 [--ci] [--tools-root PATH] [--dry-run]

--ci          Install prerequisites in a CI/container environment.
--tools-root  Use this path for local shared tools (default from TOOLS_ROOT or /home/share/tools / C:/tools).
--dry-run     Print actions without executing.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --ci)
            CI_MODE=1
            shift
            ;;
        --tools-root)
            shift
            TOOLS_ROOT="${1:-}"
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

if [[ -z "$TOOLS_ROOT" ]]; then
    TOOLS_ROOT="${TOOLS_ROOT:-$(default_tools_root)}"
fi

run_cmd() {
    if [[ "$DRY_RUN" -eq 1 ]]; then
        echo "+ $*"
    else
        eval "$*"
    fi
}

print_manifest() {
    if [[ -f "$MANIFEST_FILE" ]]; then
        echo "Using tool manifest: $MANIFEST_FILE"
        cat "$MANIFEST_FILE"
        echo
    fi
}

manifest_commands() {
    local python_exec=""
    if command -v python3 >/dev/null 2>&1; then
        python_exec=python3
    elif command -v python >/dev/null 2>&1; then
        python_exec=python
    fi

    if [[ -n "$python_exec" ]] && [[ -f "$MANIFEST_HELPER" ]]; then
        "$python_exec" "$MANIFEST_HELPER" commands "$MANIFEST_FILE"
    else
        grep -oP '^\s*command:\s*\K.*' "$MANIFEST_FILE"
    fi
}

manifest_packages() {
    local manager="$1"
    local python_exec=""
    if command -v python3 >/dev/null 2>&1; then
        python_exec=python3
    elif command -v python >/dev/null 2>&1; then
        python_exec=python
    fi

    if [[ -n "$python_exec" ]] && [[ -f "$MANIFEST_HELPER" ]]; then
        "$python_exec" "$MANIFEST_HELPER" packages "$MANIFEST_FILE" "$manager"
    else
        grep -oP '^\s*'"$manager"':\s*\K.*' "$MANIFEST_FILE"
    fi
}

check_manifest_programs() {
    if [[ ! -f "$MANIFEST_FILE" ]]; then
        return
    fi

    while IFS= read -r command; do
        if [[ -n "$command" ]]; then
            check_program "$command"
        fi
    done < <(manifest_commands)
}

check_program() {
    if command -v "$1" >/dev/null 2>&1; then
        printf "OK: %s\n" "$1"
    else
        printf "MISSING: %s\n" "$1"
    fi
}

install_packages() {
    if command -v dnf >/dev/null 2>&1; then
        local packages
        packages="$(manifest_packages dnf)"
        run_cmd "dnf install -y ${packages}"
    elif command -v apt-get >/dev/null 2>&1; then
        local packages
        packages="$(manifest_packages apt)"
        run_cmd "apt-get update -y"
        run_cmd "apt-get install -y --no-install-recommends ${packages}"
    else
        echo "No supported package manager found. Install prerequisites manually or use a prepared tool image."
        exit 1
    fi
}

echo "Using tools root: $TOOLS_ROOT"
print_manifest

if [[ "$CI_MODE" -eq 1 ]]; then
    echo "CI mode: verifying or installing prerequisites"
    if [[ $EUID -ne 0 ]]; then
        echo "CI mode requires root privileges inside the container."
        exit 1
    fi
    install_packages
    exit 0
fi

cat <<EOF
Local machine setup helper
Tools root: $TOOLS_ROOT
EOF

check_manifest_programs

cat <<EOF
If a required tool is missing, install it using your platform package manager or add it to TOOLS_ROOT.
Example Linux package managers:
  Fedora: sudo dnf install cmake ninja-build gcc-c++ gcc-arm-linux-gnu gcc-aarch64-linux-gnu arm-none-eabi-gcc arm-none-eabi-gcc-cs binutils-arm-none-eabi
  Ubuntu: sudo apt-get update && sudo apt-get install -y cmake ninja-build g++ gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf gcc-aarch64-linux-gnu g++-aarch64-linux-gnu gcc-arm-none-eabi g++-arm-none-eabi binutils-arm-none-eabi

For shared tool installs, set TOOLS_ROOT and add $TOOLS_ROOT/bin to your PATH.
EOF
