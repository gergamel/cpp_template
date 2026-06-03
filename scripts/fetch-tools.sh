#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST_HELPER="${SCRIPT_DIR}/manifest-helper.py"
PYTHON=""

if command -v python3 >/dev/null 2>&1; then
    PYTHON=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON=python
else
    echo "Python is required to run this script." >&2
    exit 1
fi

if [[ ! -f "$MANIFEST_HELPER" ]]; then
    echo "Manifest helper not found: $MANIFEST_HELPER" >&2
    exit 1
fi

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 --tool TOOL [--tools-root PATH] [--platform linux|windows] [--dry-run]"
    exit 1
fi

ARGS=()
TOOL=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --tool)
            shift
            TOOL="${1:-}"
            shift
            ;;
        --tools-root|--platform|--dry-run|-h|--help)
            ARGS+=("$1")
            if [[ "$1" == "--tools-root" || "$1" == "--platform" ]]; then
                shift
                ARGS+=("${1:-}")
            fi
            shift
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
 done

if [[ -n "$TOOL" ]]; then
    set -- "$TOOL" "${ARGS[@]}"
else
    set -- "${ARGS[@]}"
fi

exec "$PYTHON" "$MANIFEST_HELPER" fetch-tool "$@"

install_tool() {
    local tool="$1"
    local platform="$2"
    local url

    url="$(download_tool_url "$tool" "$platform")" || {
        echo "No archive install supported for tool: $tool on platform: $platform" >&2
        return 1
    }

    local temp_dir
    temp_dir="$(mktemp -d)"
    trap 'rm -rf "$temp_dir"' RETURN

    case "$tool" in
        cmake)
            install_cmake "$temp_dir" "$url"
            ;;
        ninja)
            install_ninja "$temp_dir" "$url"
            ;;
        clang)
            install_clang "$temp_dir" "$url"
            ;;
        python3)
            install_python3 "$temp_dir" "$url"
            ;;
        arm-none-eabi-gcc|arm-none-eabi-g++|arm-linux-eabihf-g++|aarch64-linux-gnu-g++)
            install_generic_toolchain "$temp_dir" "$url" "$tool"
            ;;
        *)
            echo "Unsupported archive tool: $tool" >&2
            return 1
            ;;
    esac
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --tools-root)
                TOOLS_ROOT="${2:-}"
                shift 2
                ;;
            --tool)
                SELECTED_TOOL="${2:-}"
                shift 2
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
                echo "Unknown argument: $1" >&2
                show_help
                exit 1
                ;;
        esac
    done
}

parse_args "$@"

if [[ -z "$TOOLS_ROOT" ]]; then
    TOOLS_ROOT="$(default_tools_root)"
fi

platform="$(platform_name)"
if [[ "$platform" == unknown ]]; then
    echo "Unsupported platform: $(uname -s)" >&2
    exit 1
fi

if [[ "$platform" == macos ]]; then
    echo "macOS archive install is not supported by this script yet." >&2
    exit 1
fi

mkdir -p "$TOOLS_ROOT"
mkdir -p "$TOOLS_ROOT/bin"

if [[ -n "$SELECTED_TOOL" ]]; then
    install_tool "$SELECTED_TOOL" "$platform"
else
    install_tool cmake "$platform"
    install_tool ninja "$platform"
    install_tool clang "$platform"
    if [[ "$platform" == windows ]]; then
        install_tool python3 "$platform"
    elif [[ "$platform" == linux ]]; then
        install_tool arm-none-eabi-gcc "$platform"
    fi
fi

echo
cat <<EOF
Archive install complete.
Add the following to your PATH if needed:
  $TOOLS_ROOT/bin
For extracted packages installed under subdirectories, you can also add:
  $TOOLS_ROOT/cmake/bin
  $TOOLS_ROOT/llvm/bin
  $TOOLS_ROOT/python3
  $TOOLS_ROOT/arm-none-eabi-gcc/bin
EOF
