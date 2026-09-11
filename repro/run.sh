#!/bin/sh
# Build the standalone reproducer with stock minizip CMake targets.
set -eu

if [ "$#" -gt 1 ]; then
    echo "Usage: $0 [zlib|apple|bzip2|lzma|xz|zstd|ppmd]" >&2
    exit 2
fi
backend=${1:-zlib}
libcomp=OFF
bzip2=OFF
lzma=OFF
zstd=OFF
ppmd=OFF
method=deflate
case "$backend" in
    zlib) ;;
    bzip2) bzip2=ON; method=bzip2 ;;
    lzma|xz) lzma=ON; method=$backend ;;
    zstd) zstd=ON; method=zstd ;;
    ppmd) ppmd=ON; method=ppmd ;;
    apple)
        if [ "$(uname -s)" != Darwin ]; then
            echo "The Apple Compression backend requires an Apple platform." >&2
            exit 2
        fi
        libcomp=ON
        ;;
    *) echo "Usage: $0 [zlib|apple|bzip2|lzma|xz|zstd|ppmd]" >&2; exit 2 ;;
esac

cmake_command=${CMAKE:-cmake}
if ! command -v "$cmake_command" >/dev/null 2>&1; then
    echo "CMake was not found: $cmake_command" >&2
    exit 2
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 2
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/minizip-repro.XXXXXX") || exit 2
trap 'rm -rf "$build_dir"' 0
trap 'exit 2' HUP INT TERM

"$cmake_command" -S "$script_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DMZ_LIBCOMP="$libcomp" -DMZ_ZLIB=ON -DMZ_ZLIB_FLAVOR=zlib \
    -DMZ_BZIP2="$bzip2" -DMZ_LZMA="$lzma" -DMZ_PPMD="$ppmd" -DMZ_ZSTD="$zstd" \
    -DFETCHCONTENT_SOURCE_DIR_PPMD="${PPMD_SOURCE_DIR:-}" \
    -DMZ_OPENSSL=OFF -DMZ_FETCH_LIBS=OFF \
    -DMZ_BUILD_TESTS=OFF -DMZ_BUILD_UNIT_TESTS=OFF || exit 2
"$cmake_command" --build "$build_dir" --config Release \
    --target minizip_repro --parallel 4 || exit 2

# Preserve the C program's exit status, including 1 for the reproduced defect.
"$build_dir/bin/minizip_repro" "$method"
