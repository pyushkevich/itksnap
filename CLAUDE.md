# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Layout

One or more build directories sit **next to** `itksnap/` (not inside it). Their names reflect the platform/config, e.g. `xc64dbg`, `xc64rel`, `vc22`, `vce64dbg`. All source lives under `itksnap/`. Builds are always out-of-source.

## Build Commands

Build from an existing configured build directory (replace `<build>` with the actual directory):

```sh
# Single-config generators (Ninja, Unix Makefiles)
cmake --build <build> --parallel

# Multi-config generators (Visual Studio) — specify --config
cmake --build <build> --config Debug --parallel
cmake --build <build> --config Release --parallel

# Build a specific target
cmake --build <build> --target ITK-SNAP --parallel
cmake --build <build> --target itksnap-wt --parallel
```

Configure a new build from scratch (requires ITK ≥ 5.4, VTK ≥ 9.3.1, Qt6):
```sh
cmake -G <generator> <path-to-itksnap-source> \
  -DITK_DIR=<itk-build> -DVTK_DIR=<vtk-build> \
  -DQt6_DIR=<qt6-prefix>/lib/cmake/Qt6
```

After cloning, submodules must be initialized:
```sh
git submodule init && git submodule update
```

## Running Tests

Tests use CTest (CDash dashboard: `my.cdash.org`, project `ITK-SNAP`):

```sh
cd <build>
ctest                          # run all tests (single-config)
ctest -C Debug                 # run all tests (multi-config)
ctest -R IRISApplicationTest   # run a specific test by name
ctest -R testRLE               # run a specific test
ctest --output-on-failure      # show output for failing tests

# GUI tests need a display on Linux:
xvfb-run -a ctest -R Workspace
```

GUI tests are launched as `ITK-SNAP --test <TestID> --testdir <TestData>` and have a 180-second timeout. Set `SNAP_GUI_TEST_ACCEL` lower than `1.0` on slow machines.

## Architecture: Three-Tier MVP with ITK Events

The codebase follows a strict three-tier architecture. Dependencies only flow downward.

### Tier 1 — Logic (`Logic/`)

Pure image processing, no GUI dependency. All classes inherit from `itk::Object` and use `itk::SmartPointer<T>` (aliased as `SmartPtr<T>`). The central class is `IRISApplication` (`Logic/Framework/IRISApplication.h`), which owns image data, manages undo/redo, and coordinates all application operations.

When state changes, logic classes fire typed `itk::EventObject` subclasses defined in `Common/SNAPEvents.h`.

### Tier 2 — UI Model (`GUI/Model/`)

Toolkit-independent presenter layer. All models inherit from `AbstractModel` (`Common/AbstractModel.h`), which itself inherits `itk::Object`. Models subscribe to Logic events via `Rebroadcast(src, srcEvent, trgEvent)` and translate them into UI-relevant state.

Values exposed to the View are wrapped in `PropertyModel<T>` or `AbstractRangedPropertyModel<T>` (for sliders with min/max/step). These fire a `ModelUpdateEvent` when their value or domain changes. The top-level model is `GlobalUIModel` (`GUI/Model/GlobalUIModel.h`), instantiated once in `main.cxx`.

An `EventBucket` collects all fired events between `Update()` calls so models process a batch at once.

### Tier 3 — Qt View (`GUI/Qt/`)

Qt widgets and windows. The `GUI/Qt/Coupling/` system provides `AbstractWidgetDataMapping` that binds a Qt widget to a `PropertyModel` bidirectionally:
- Model change → `UpdateWidgetFromModel(EventBucket)`
- User interaction → `UpdateModelFromWidget()`

Typical usage is a one-liner: `makeCoupling(ui->mySpinBox, myModel)`.

## Key Subsystems

**Image Layers** (`Logic/ImageWrapper/`): `ImageWrapper<TImage>` wraps an ITK image with display, slicing, and IO. Specialized as `ScalarImageWrapper`, `VectorImageWrapper`, `LabelImageWrapper`. Iterate over layers with `LayerIterator` by role (`MAIN_ROLE`, `OVERLAY_ROLE`, `LABEL_ROLE`).

**RLE Label Images** (`Logic/RLEImage/`): Custom ITK-compatible image type using run-length encoding for memory-efficient storage of segmentation label images. Fully interoperable with ITK iterators.

**Slice Rendering** (`GUI/Renderer/`): `AbstractVTKRenderer` / `AbstractVTKSceneRenderer` decouple VTK pipelines from Qt widgets. 2D slices use a custom OpenGL2 renderer (new in 4.4); 3D view uses VTK.

**Interaction Modes**: Slice views use a strategy pattern — `CrosshairsInteractionMode`, `PaintbrushInteractionMode`, etc., all inherit from a common interface; `InteractionModeClient` manages the active mode.

**Workspace / Remote** (`Logic/WorkspaceAPI/`): Workspace file format, REST client, and SSH tunneling for the distributed deep learning segmentation (DLS) framework.

**Submodules** (`Submodules/`):
- `c3d` → image algebra library (`cnd_api`, `cnd_driver`) and `c3d` CLI
- `greedy` → deformable image registration library (`greedyapi`) and `greedy` CLI
- `digestible` → T-Digest approximate quantile statistics

## Executables

| Target | Entry Point | Description |
|--------|-------------|-------------|
| `ITK-SNAP` | `GUI/Qt/main.cxx` | Main GUI application |
| `itksnap-wt` | `Utilities/Workspace/WorkspaceTool.cxx` | CLI workspace manipulation |

CLI flags for the GUI: `-g` (main image), `-s` (segmentation), `-o` (overlays), `-w` (workspace), `--test TESTID` (automated test), `--lang`, `--threads`, `--scale`.

`--url URL` is the flag used by the Windows URL-scheme registry handler (`shell/open/command`). It triggers single-instance forwarding: image URLs are sent to the first already-running ITK-SNAP instance via IPC and the new process exits immediately; workspace URLs (`.itksnap`) always open a new window. The NSIS installer registers all `itksnap-*` schemes with `ITK-SNAP.exe --url "%1"`.

## CMakeLists.txt: Registering New Headers

Whenever a new header file is added to the source tree it must also be added to `CMakeLists.txt` so that QtCreator's file browser and search index include it. There are three lists:

- **`LOGIC_HEADERS`** (around line 320) — headers under `Common/`, `Logic/`, and any toolkit-independent code.
- **`UI_MOC_HEADERS`** (around line 800) — Qt widget/model headers that contain `Q_OBJECT` or `Q_GADGET` and must be processed by `moc`.
- **`UI_NONMOC_HEADERS`** (around line 885) — Qt-related headers that do *not* need `moc` (pure interfaces, coupling helpers, delegates without signals).

Within each list, entries are sorted alphabetically by path. The `.cxx` source files are registered in separate `SET(...)` blocks (`SNAP_CXX`, `UI_QT_CXX`, etc.) that are already nearby in the file.

## Common Patterns

- Use `SmartPtr<T>` (= `itk::SmartPointer<T>`) and `irisNew<T>(...)` for logic objects; raw `new` only for Qt-owned widgets.
- `irisITKObjectMacro(ClassName, SuperClass)` declares the class boilerplate (New, Self, Superclass typedefs).
- `Registry` (`Common/Registry.h`) is used for all INI-style settings files (`.itksnap` workspace format).
- The `PropertyModel` pattern is the correct way to expose any settable value from a UI model to a widget — do not bypass it with direct getter/setter calls into logic.
- Vector math uses `vnl_vector_fixed`-based types defined in `Common/IRISVectorTypes.h` (`Vector3d`, `Vector3ui`, etc.).
- **ITK→Qt event routing**: always connect ITK events to `onModelUpdate(EventBucket)` (via `LatentITKEventNotifier::connect`), then dispatch inside `onModelUpdate` using `b.HasEvent(SomeEvent(), source)`. Never connect to a dedicated slot that ignores the bucket — the bucket coalesces batched events and prevents duplicate handling.
