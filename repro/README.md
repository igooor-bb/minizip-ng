# Finalization reproducer

This is a portable C reproducer using minizip's public API and in-memory streams. The zlib, BZip2, liblzma, Zstd and PPMd fixes are platform-independent. The Apple Compression fix applies where that backend is available. Actual runs so far were on macOS.

## Build and run (verified on macOS)

Requires a POSIX shell, CMake, a C compiler, a build tool supported by CMake, and zlib for the zlib configuration. Run from the repository root:

```sh
./repro/run.sh zlib

# Immediately after the invocation:
printf 'exit=%s\n' "$?"
```

Select the backend with `./repro/run.sh zlib`, `./repro/run.sh apple`, `./repro/run.sh bzip2`, `./repro/run.sh lzma`, `./repro/run.sh xz`, `./repro/run.sh zstd` or `./repro/run.sh ppmd`. LZMA and XZ both use liblzma. Omitting the argument selects zlib.

BZip2, liblzma and Zstd require their development libraries. CMake can locate nonstandard installations through `CMAKE_PREFIX_PATH`. PPMd uses the 7-Zip sources fetched by stock minizip CMake. Set `PPMD_SOURCE_DIR` to an existing 7-Zip 26.02 checkout to build PPMd without downloading sources.

The script builds in a temporary directory and removes it on exit. The small CMake wrapper uses stock minizip targets to supply headers and link dependencies. Set `CMAKE=/path/to/cmake` if CMake is not on PATH. The same C source and assertions are used before and after. Builds on other platforms have not been validated.

Exit status:

- **0**: correct behavior.
- **1**: target defect reproduced.
- **2**: scenario preparation, configuration, or build failure.

The example writes seven bytes using the selected compression method. Immediately before entry close, the custom public output stream rejects exactly one write with MZ_WRITE_ERROR. All subsequent writes succeed. Entry close must return MZ_WRITE_ERROR (-116), not MZ_OK (0). Failure injection must fire exactly once. Otherwise the scenario is invalid. Archive close is cleanup only: after an entry finalization failure the archive is incomplete, and its close result is not the assertion under test.

All backends must return MZ_WRITE_ERROR (-116). The before branch returns MZ_OK and exits 1. The after branch returns MZ_WRITE_ERROR and exits 0.
