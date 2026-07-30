#!/usr/bin/env bash
#
# Builds a release .pdx and packages it into releaseBuilds/.
#
# Bumps buildNumber in Source/pdxinfo, builds both the device (ARM) and
# simulator (Release) binaries, verifies the packaged pdx, zips it into
# releaseBuilds/, and commits the pdxinfo bump.
#
# Usage:
#   ./tools/release.sh
#   ./tools/release.sh --version 0.9.1
#   ./tools/release.sh --no-commit

set -euo pipefail

GAME_NAME='RunicRogues'
VERSION=''
NO_COMMIT=0

while [ $# -gt 0 ]; do
    case "$1" in
        --version)
            VERSION="${2:-}"
            shift 2
            ;;
        --version=*)
            VERSION="${1#*=}"
            shift
            ;;
        --no-commit)
            NO_COMMIT=1
            shift
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
pdxinfo_path="$repo_root/Source/pdxinfo"
pdx_dir="$repo_root/$GAME_NAME.pdx"
output_dir="$repo_root/releaseBuilds"

# Both builds write into Source/ and re-run pdc over it, so they must run
# sequentially — in parallel they race on the same .pdx output directory.
presets=(device sim-release)

is_tty=0
if [ -t 1 ]; then
    is_tty=1
fi

write_step() {
    echo ""
    if [ "$is_tty" -eq 1 ]; then
        printf '\033[36m==> %s\033[0m\n' "$1"
    else
        echo "==> $1"
    fi
}

write_warn() {
    if [ "$is_tty" -eq 1 ]; then
        printf '\033[33m%s\033[0m\n' "$1"
    else
        echo "$1"
    fi
}

# ---------------------------------------------------------------- version bump

if [ ! -f "$pdxinfo_path" ]; then
    echo "Could not find $pdxinfo_path" >&2
    exit 1
fi

original_pdxinfo="$(cat "$pdxinfo_path")"

old_build_number="$(printf '%s\n' "$original_pdxinfo" | sed -n 's/^buildNumber=[ \t]*\([0-9][0-9]*\).*/\1/p' | head -n1)"
if [ -z "$old_build_number" ]; then
    echo "No 'buildNumber=' line in Source/pdxinfo." >&2
    exit 1
fi
new_build_number=$((old_build_number + 1))

new_version="$(printf '%s\n' "$original_pdxinfo" | sed -n 's/^version=[ \t]*\(.*\)/\1/p' | head -n1)"
new_version="$(printf '%s' "$new_version" | sed 's/[[:space:]]*$//')"
if [ -z "$new_version" ]; then
    echo "No 'version=' line in Source/pdxinfo." >&2
    exit 1
fi

if [ -n "$VERSION" ]; then
    if ! printf '%s' "$VERSION" | grep -Eq '^[0-9]+(\.[0-9]+){1,2}$'; then
        echo "Version '$VERSION' must look like 1.2 or 1.2.3." >&2
        exit 1
    fi
    new_version="$VERSION"
fi

restore_pdxinfo() {
    printf '%s' "$original_pdxinfo" > "$pdxinfo_path"
}

# Rewrite the buildNumber and version lines in place.
awk -v bn="$new_build_number" -v ver="$new_version" '
    /^buildNumber=/ { print "buildNumber=" bn; next }
    /^version=/     { print "version=" ver;   next }
    { print }
' "$pdxinfo_path" > "$pdxinfo_path.tmp"
mv "$pdxinfo_path.tmp" "$pdxinfo_path"

write_step "Release $new_version (build $old_build_number -> $new_build_number)"

# ---------------------------------------------------------------------- builds

pkill -f 'Playdate Simulator' 2>/dev/null && \
    echo "Stopped the Playdate Simulator (it holds a lock on the pdx)." || true

build_failed() {
    restore_pdxinfo
    echo ""
    write_warn "Build failed - reverted Source/pdxinfo to buildNumber=$old_build_number."
}

for preset in "${presets[@]}"; do
    if [ ! -f "$repo_root/build/build-$preset/CMakeCache.txt" ]; then
        write_step "Configuring '$preset'"
        if ! cmake --preset "$preset"; then
            build_failed
            exit 1
        fi
    fi

    write_step "Building '$preset'"
    if ! cmake --build --preset "$preset"; then
        build_failed
        exit 1
    fi
done

# --------------------------------------------------------------- verify output

write_step "Verifying the packaged pdx"

if [ ! -d "$pdx_dir" ]; then
    echo "Builds finished but $pdx_dir does not exist." >&2
    exit 1
fi

# device -> pdex.bin, sim-release -> pdex.dylib (macOS simulator binary)
if [ ! -f "$pdx_dir/pdex.bin" ]; then
    echo "pdex.bin is missing from the pdx - the 'device' build did not produce it." >&2
    exit 1
fi
if [ ! -f "$pdx_dir/pdex.dylib" ]; then
    echo "pdex.dylib is missing from the pdx - the 'sim-release' build did not produce it." >&2
    exit 1
fi

# pdc only re-runs as a post-build step, so a skipped relink would silently
# leave the previous build's pdxinfo inside the pdx.
built_build="$(sed -n 's/^buildNumber=[ \t]*\([0-9][0-9]*\).*/\1/p' "$pdx_dir/pdxinfo" | head -n1)"
built_version="$(sed -n 's/^version=[ \t]*\(.*\)/\1/p' "$pdx_dir/pdxinfo" | head -n1)"
built_version="$(printf '%s' "$built_version" | sed 's/[[:space:]]*$//')"

if [ "$built_build" != "$new_build_number" ] || [ "$built_version" != "$new_version" ]; then
    echo "The pdx is stale: it has version=$built_version buildNumber=$built_build, expected version=$new_version buildNumber=$new_build_number. pdc did not re-run." >&2
    exit 1
fi

echo "pdex.bin + pdex.dylib present, pdxinfo reports $built_version ($built_build)."

# ------------------------------------------------------------------- packaging

write_step "Packaging"

mkdir -p "$output_dir"

zip_path="$output_dir/$GAME_NAME$new_version.pdx.zip"
if [ -f "$zip_path" ]; then
    rm -f "$zip_path"
    echo "Overwrote the existing $(basename "$zip_path")."
fi

# zip from the pdx's parent so entries are stored as "RunicRogues.pdx/..."
# with forward slashes and the folder tree intact.
(
    cd "$(dirname "$pdx_dir")"
    zip -r -q -X "$zip_path" "$(basename "$pdx_dir")"
)

# ---------------------------------------------------------------------- commit

if [ "$NO_COMMIT" -eq 0 ]; then
    if ! command -v git >/dev/null 2>&1; then
        write_warn "git is not on PATH - skipping the commit."
    else
        message="Release v$new_version (build $new_build_number)"
        if git -C "$repo_root" commit -q -m "$message" -- 'Source/pdxinfo'; then
            echo "Committed: $message"
        else
            write_warn "Could not commit Source/pdxinfo - commit it manually."
        fi
    fi
fi

# stat -f is BSD/macOS.
size_mb="$(awk -v b="$(stat -f%z "$zip_path")" 'BEGIN { printf "%.1f", b / 1048576 }')"
echo ""
if [ "$is_tty" -eq 1 ]; then
    printf '\033[32mRelease ready: %s (%s MB)\033[0m\n' "$zip_path" "$size_mb"
else
    echo "Release ready: $zip_path ($size_mb MB)"
fi
