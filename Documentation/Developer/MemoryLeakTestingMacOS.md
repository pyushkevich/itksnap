# Memory Leak Testing on macOS

This document explains how to build ITK-SNAP in a leak-detectable configuration
and use the macOS `leaks` tool to find heap memory leaks in the GUI test suite.

---

## Why Not AddressSanitizer?

AddressSanitizer (ASan) replaces the system memory allocator, which makes the
heap opaque to macOS's `leaks` tool. The two approaches are mutually exclusive.
For leak detection use a plain `Debug` (or `RelWithDebInfo`) build **without**
any sanitizer flags.

---

## 1. Build a Leak-Testable Binary

Configure CMake without sanitizers and with debug info:

```bash
mkdir build-leaks && cd build-leaks
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DITK_DIR=/path/to/itk/build \
  -DVTK_DIR=/path/to/vtk/lib/cmake/vtk-9.3 \
  ../itksnap
ninja ITK-SNAP
```

A `RelWithDebInfo` build also works and runs faster, but stack traces are
occasionally less precise due to inlining.

---

## 2. Sign the Binary for `leaks`

macOS's `leaks --atExit` requires the `com.apple.security.get-task-allow`
entitlement. Without it the tool silently produces no output. Re-apply this
signature after every `ninja` invocation that relinks the binary:

```bash
codesign --force -s - --entitlements /dev/stdin build-leaks/ITK-SNAP <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>com.apple.security.get-task-allow</key>
  <true/>
</dict>
</plist>
EOF
```

> **Note:** The ad-hoc signature (`-s -`) is sufficient for local testing.
> It does not affect code-signing for distribution builds.

---

## 3. Run a Single GUI Test

ITK-SNAP's GUI tests are driven by `--test <TestName>`:

```bash
MallocStackLogging=1 leaks --atExit -- build-leaks/ITK-SNAP \
  --test PreferencesDialog \
  --testdir itksnap/Testing/TestData
```

`MallocStackLogging=1` tells the allocator to record the call stack for every
allocation. This is required for `leaks` to produce useful stack traces, but it
slows allocation by ~2–3×.

The test exits automatically when the test script finishes. `leaks --atExit`
intercepts the `exit()` call, inspects the live heap, and prints a report
before the process terminates.

Exit code: **0** = no leaks found, **1** = leaks found.

---

## 4. Read the Output

```
Process 12345: 564 leaks for 83888 total leaked bytes.
...
ROOT LEAK: <vtkScalarBarActor 0x...> [208 bytes]
  ...
  Call stack:
    vtkObjectBase::New()
    vtkScalarBarActor::New()
    Generic3DRenderer::Generic3DRenderer()
    ...
```

Key fields:

| Field | Meaning |
|---|---|
| **ROOT LEAK** | Object with no live references — a true leak |
| **ROOT CYCLE** | Group of objects pointing to each other in a cycle, unreachable from any live root |
| Call stack | Where the leaked object was allocated |

Focus on ROOT LEAKs and ROOT CYCLEs whose call stacks lead into ITK-SNAP
source files. Leaks rooted in Qt internals (`QStandardItem`,
`QObjectPrivate::ConnectionData`, `QHashPrivate`, etc.) are framework-owned
and not addressable from ITK-SNAP code.

---

## 5. Run All GUI Tests

The GUI test names (defined in `CMakeLists.txt` under `add_test`) can be
passed one at a time:

```bash
TESTDIR=itksnap/Testing/TestData
BINARY=build-leaks/ITK-SNAP

for TEST in PreferencesDialog RandomForestBailOut Workspace \
            EchoCartesianDicomLoading MeshImport MeshWorkspace \
            SegmentationMesh VolumeRendering LabelSmoothing \
            NaNs DiffSpace Reloading RegionCompetition RandomForest; do
  LEAKS=$(MallocStackLogging=1 leaks --atExit -- "$BINARY" \
            --test "$TEST" --testdir "$TESTDIR" 2>&1 \
          | grep "leaks for" \
          | grep -oE "[0-9]+ leak" | head -1 | grep -oE "^[0-9]+")
  printf "%-30s %s leaks\n" "$TEST" "${LEAKS:-ERROR}"
done
```

Non-GUI (logic-layer) tests use CTest directly and produce no leaks on a clean
build:

```bash
cd build-leaks
MallocStackLogging=1 ctest -R "IRISApplicationTest|TDigest|BasicSlicing" -V
```

---

## 6. Expected Baseline (After All Known Fixes)

A clean build on the `bug/memory-leak` branch reports approximately:

| Test | Leaks | Notes |
|---|---:|---|
| `PreferencesDialog` | ~564 | Qt ROOT CYCLEs only |
| `RandomForestBailOut` | ~564 | Qt ROOT CYCLEs only |
| `Workspace` | ~2,000 | Lifecycle leaks (no teardown) |
| `EchoCartesianDicomLoading` | ~2,000 | Lifecycle leaks |
| `MeshImport` | ~1,700 | Lifecycle leaks |
| `RandomForest` | ~14,000 | Includes RandomForest decision-tree data |

Any test that previously showed non-deterministic spikes (e.g.
`EchoCartesianDicomLoading` jumping to ~90,000 leaks) should now be stable.
A large or growing count in `PreferencesDialog` or `RandomForestBailOut` is
the clearest indicator of a new regression.

---

## 7. Tips

- **Re-sign after every relink.** `ninja` relinks `ITK-SNAP` when any
  translation unit changes; the new binary loses the entitlement and `leaks`
  will silently produce no output until you re-sign.

- **Non-deterministic counts** (a test shows 2 K on one run and 90 K on
  another) typically indicate a reference cycle that is only triggered by a
  certain event-processing order. Run the test 5–10 times and look for the
  high-water mark.

- **Narrow a leak to its source.** If a leak appears in multiple tests,
  comment out initialisation code paths incrementally (or add `printf`/log
  statements around suspected `New()` calls) to bisect the call site.

- **`leaks` vs. Instruments.** Xcode Instruments (Allocations + Leaks
  template) provides the same information with a GUI. Use `leaks` for
  scripted / CI-style checks and Instruments for exploratory investigation.
