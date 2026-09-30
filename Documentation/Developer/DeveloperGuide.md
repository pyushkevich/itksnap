# ITK-SNAP Developer Guide

This guide is for people who want to build ITK-SNAP from source and change it. It covers
getting the code, building it, running the tests, and finding your way around the
codebase.

If you are looking for how to *submit* a change, see
[CONTRIBUTING.md](../../CONTRIBUTING.md). For who reviews and merges changes, see
[GOVERNANCE.md](../../GOVERNANCE.md).

**Contents**

- [Getting the source](#getting-the-source)
- [Dependencies](#dependencies)
- [Building](#building)
- [Running ITK-SNAP](#running-itk-snap)
- [Running the tests](#running-the-tests)
- [Architecture](#architecture)
- [Key patterns](#key-patterns)
- [Where to make common changes](#where-to-make-common-changes)
- [Coding style](#coding-style)
- [Further reading](#further-reading)

---

## Getting the source

ITK-SNAP uses git submodules (`Submodules/c3d`, `Submodules/greedy`,
`Submodules/digestible`), so clone recursively:

```bash
git clone --recursive https://github.com/pyushkevich/itksnap.git
```

If you already cloned without `--recursive`, fill in the submodules:

```bash
git submodule update --init --recursive
```

A build configured against a repository with empty submodule directories will fail in
confusing ways. If CMake reports missing sources under `Submodules/`, this is why.

## Dependencies

ITK-SNAP is C++17 and requires CMake ≥ 3.16.

| Dependency  | Minimum   | Notes                                                              |
| ----------- | --------- | ------------------------------------------------------------------ |
| ITK         | 5.4       | Built with the components ITK-SNAP requires; see `CMake/standalone.cmake` |
| VTK         | 9.3.1     | Must include the `RenderingExternal` module (used by the 3D view)  |
| Qt          | 6         | Components: Widgets, OpenGL, Concurrent, Qml, LinguistTools        |
| libcurl     | —         | Remote image loading                                                |
| libssh      | —         | SSH tunneling to remote deep-learning servers                       |

Continuous integration builds against **ITK v5.4.0, VTK 9.5.2, and Qt 6.8.1** on Linux,
macOS (Intel and Apple Silicon), and Windows. Those versions are the best-tested
combination; the table above gives the floors that CMake enforces.

Two dependency notes that cost people time:

- **VTK must be built with `-DVTK_MODULE_ENABLE_VTK_RenderingExternal=YES`.** The 3D
  render widget (`QtFrameBufferOpenGLWidget`) uses `vtkExternalOpenGLRenderWindow`. A VTK
  build without this module configures fine and fails at link time.
- **Use a consistent ITK build.** ITK uses `extern template`, so ITK-SNAP relies entirely
  on the compiled ITK libraries rather than on headers alone. An ITK build directory whose
  sources were updated without recompiling will produce undefined-symbol link errors that
  look like ITK-SNAP bugs but are not.

Prebuilt-dependency instructions for each platform are maintained on
[the ITK-SNAP website](http://www.itksnap.org/pmwiki/pmwiki.php?n=Documentation.BuildingITK-SNAP).

## Building

ITK-SNAP builds out of source. Ninja is recommended:

```bash
mkdir build && cd build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DITK_DIR=/path/to/itk/lib/cmake/ITK-5.4 \
  -DVTK_DIR=/path/to/vtk/lib/cmake/vtk-9.5 \
  -DCMAKE_PREFIX_PATH=/path/to/qt6 \
  ../itksnap
ninja
```

Useful targets:

```bash
ninja ITK-SNAP     # the GUI application only
ninja              # the "all" target: application, CLI tools, and test executables
```

**Build `all`, not just `ITK-SNAP`, before opening a pull request.** The test executables
and the bundled command-line tools are only built by `all`, so `ninja ITK-SNAP` alone can
leave a compile error undiscovered until CI finds it.

**Use a fresh build directory when you change a dependency's major or minor version.**
Reconfiguring an existing build tree across, say, a VTK 9.3 → 9.5 upgrade leaves stale
cached paths and produces failures that are hard to attribute.

## Running ITK-SNAP

```bash
./ITK-SNAP                       # or ITK-SNAP.app/Contents/MacOS/ITK-SNAP on macOS
./ITK-SNAP -g image.nii.gz       # load a main image
./ITK-SNAP -w workspace.itksnap  # load a workspace
```

On Linux without a display (for example over SSH), run under a virtual framebuffer:

```bash
xvfb-run -a ./ITK-SNAP
```

## Running the tests

Tests are registered with CTest and run from the build directory:

```bash
ctest                          # everything
ctest -R BasicSlicingTestX39   # one test by name
ctest -V                       # verbose output
ctest -j4                      # in parallel
```

On headless Linux, GUI tests need a virtual framebuffer:

```bash
xvfb-run -a ctest
```

The suite has two kinds of test:

**Logic and unit tests** are standalone executables (`logic_api_test`, `testTDigest`,
`TestLargeImageCheck`, the slicing tests) built from `Testing/Logic/` and driven by
`itkTestDriver`.

**GUI tests** drive the real application. Each is registered from the `GUI_TESTS` list in
the top-level `CMakeLists.txt` and runs as:

```bash
./ITK-SNAP --test <TestName> --testdir <path-to>/Testing/TestData
```

The runner loads the script `Testing/GUI/Qt/Scripts/test_<TestName>.js`, compiled into the
binary as a Qt resource via `Testing/GUI/Qt/TestingScripts.qrc`. To add a GUI test, add
the script, register it in the `.qrc`, and add its name to `GUI_TESTS`. **The name in
`GUI_TESTS` must match the script filename exactly** — a mismatch makes the test look for
a script that does not exist, and a GUI test with no script reports as passing without
running anything.

Test data lives in `Testing/TestData/`. GUI tests have a 180-second timeout; on a slow
machine, or under software rendering, set the CMake variable `SNAP_GUI_TEST_ACCEL` below
`1.0` to slow the scripted interaction down proportionally.

A few tests are sensitive to their environment. Tests under the `Remote` label need
network access, and timing-sensitive GUI tests (notably the 4D replay and rendering tests)
can fail under software OpenGL on a slow machine without indicating a real regression. If
a test fails, check whether it also fails on unmodified `master` before assuming your
change caused it.

## Architecture

ITK-SNAP is organized in three layers, with a shared infrastructure layer beneath them.
The dependency direction is strictly one way: **Qt code may depend on models, models may
depend on logic, and logic depends on neither.**

```
GUI/Qt/       Qt widgets, views, windows          — Qt, no ITK/VTK algorithms
GUI/Renderer/ VTK and OpenGL scene renderers      — no Qt widget dependency
GUI/Model/    presentation models                 — no Qt
Logic/        image data, algorithms, IO          — no Qt, no GUI
Common/       events, properties, shared types    — used by all of the above
```

Keeping `Logic/` free of Qt is a deliberate design decision, not an accident: it is what
allows ITK-SNAP's core to be used from command-line tools and other front ends.

### `Logic/` — computation and data

No GUI dependencies at all.

- `Logic/Framework/IRISApplication.cxx` — top-level application state; the entry point to
  almost everything. Start reading here.
- `Logic/Framework/IRISImageData.cxx`, `SNAPImageData.cxx` — the image containers for the
  two application modes (see below).
- `Logic/ImageWrapper/` — the `ImageWrapper<T>` abstraction over ITK images, plus display
  policies, histograms, and slicing.
- `Logic/Slicing/` — the 2D slice-extraction pipeline, OpenGL2-accelerated with a software
  fallback.
- `Logic/LevelSet/` — the snake/level-set segmentation algorithms.
- `Logic/Mesh/` — VTK-based mesh generation and processing.
- `Logic/Preprocessing/` — speed-image and preprocessing filters.
- `Logic/WorkspaceAPI/` — the public API for reading and manipulating workspaces; also used
  by the `itksnap-wt` command-line tool.
- `Logic/RLEImage/` — run-length-encoded label image storage.

### `GUI/Model/` — presentation models

Mediates between `Logic/` and Qt widgets. Models derive from `AbstractModel` and expose
their state as observable properties. **This layer must not include Qt widget headers**;
it communicates outward through the property and event system, which is what keeps the
GUI replaceable.

### `GUI/Qt/` — presentation

- `Windows/` — top-level windows and dialogs (`MainImageWindow` is the main one).
- `Components/` — reusable widgets.
- `View/` — the OpenGL-backed 2D slice views and 3D view.
- `Coupling/` — the machinery binding model properties to Qt widgets (see below).
- `ModelView/` — Qt item-model adapters.
- `Resources/`, `Translations/` — icons, `.qrc` resources, and translation files.
- `main.cxx` — application entry point.

Slice-view interaction uses a strategy pattern: `CrosshairsInteractionMode`,
`PaintbrushInteractionMode`, and their siblings share a common interface, and
`InteractionModeClient` manages which one is active.

### `GUI/Renderer/` — rendering

`AbstractVTKRenderer` and `AbstractVTKSceneRenderer` decouple the VTK pipelines from the
Qt widgets that host them. The 3D view renders through VTK; 2D slices use a custom OpenGL2
renderer introduced in 4.4, with a software fallback for devices that cannot support it.

### `Common/` — shared infrastructure

- `AbstractModel.h` — base class for all models; ITK-style event firing.
- `PropertyModel.h` — the typed, observable property system. Central to the whole GUI
  model layer, and the single most useful header to understand.
- `SNAPEvents.h` — event type definitions.
- `Registry.cxx` — the hierarchical settings and serialization format used for
  preferences and workspaces.
- `IRISException.h` — the project's exception type.

### `Submodules/`

- `c3d/` — the Convert3D command-line tool.
- `greedy/` — the Greedy deformable registration tool, used for registration and
  segmentation propagation.
- `digestible/` — t-digest, for approximate quantiles and histograms.

## Key patterns

### The property and coupling system

This is the pattern you must understand to work on the GUI.

A model exposes state as an `AbstractPropertyModel<T>` — a value that can be read, set,
and observed. A *coupling* in `GUI/Qt/Coupling/` binds that property to a Qt widget in
both directions: editing the widget writes through to the model, and a change in the model
updates the widget. The model never mentions Qt, and the widget never reaches into the
logic layer.

In practice, a new user-visible option means: add the property to the relevant model in
`GUI/Model/`, then make a single coupling call in the corresponding widget class rather
than wiring signals and slots by hand.

Changes propagate through ITK-style events defined in `Common/SNAPEvents.h`; events are
batched in an `EventBucket` so that a burst of related changes causes one update rather
than many.

### `ImageWrapper`

All image data is reached through `ImageWrapperBase` and its subclasses —
`ScalarImageWrapper`, `VectorImageWrapper`, `LabelImageWrapper`. A wrapper owns a typed
ITK image and adds what the application needs on top of it: display mapping, slice
extraction, histograms, and metadata. Application code should work with wrappers rather
than with raw `itk::Image` pointers.

### IRIS mode and SNAP mode

ITK-SNAP has two modes, and the distinction runs through the whole codebase:

- **IRIS mode** — manual segmentation with the paintbrush, polygon, and other tools;
  image data lives in `IRISImageData`.
- **SNAP mode** — semi-automatic level-set segmentation on a sub-region of interest;
  image data lives in `SNAPImageData`.

`IRISApplication` owns both and manages the transition between them. When adding a
feature, be explicit about which mode it applies to.

### Memory ownership

ITK-SNAP uses ITK smart pointers extensively, and the mixture of owning and non-owning
pointers has historically been a source of leaks. Before adding new object relationships,
read [MemoryManagement.md](MemoryManagement.md) — it documents the ownership patterns that
caused past leaks and how to avoid repeating them.

## Where to make common changes

| You want to…                       | Start at                                                                 |
| ---------------------------------- | ------------------------------------------------------------------------ |
| Add a user-visible option          | The relevant model in `GUI/Model/`, then a coupling in the widget class   |
| Add a menu item or dialog          | `GUI/Qt/Windows/`, plus a model to hold its state                         |
| Change how a slice is rendered     | `Logic/Slicing/`, `GUI/Renderer/`, and `GUI/Qt/View/`                     |
| Add or change an image IO path     | `Logic/ImageWrapper/` and the IO classes in `Logic/Framework/`            |
| Add a segmentation algorithm       | `Logic/LevelSet/` or `Logic/Preprocessing/`                               |
| Change workspace contents          | `Logic/WorkspaceAPI/` and `Common/Registry.cxx` — and read the note below |
| Add a command-line option          | `GUI/Qt/main.cxx` and `Common/CommandLineArgumentParser.cxx`              |
| Add a test                         | `Testing/Logic/` for unit tests, `Testing/GUI/Qt/Scripts/` for GUI tests  |

**Workspaces and file formats deserve extra care.** ITK-SNAP opens workspaces written by
much older versions, and users rely on that. Adding a field is usually safe; changing or
removing the meaning of an existing one is not, and should be discussed in an issue first.

### Registering a new header in CMakeLists.txt

This one is easy to miss, because the build succeeds without it. When you add a header
file, also add it to the appropriate list in the top-level `CMakeLists.txt`, so that it
appears in IDE file browsers and search indexes and — for Qt headers — gets processed by
`moc`:

| List                 | For                                                                     |
| -------------------- | ----------------------------------------------------------------------- |
| `LOGIC_HEADERS`      | Headers under `Common/` and `Logic/`, and other toolkit-independent code |
| `UI_MOC_HEADERS`     | Qt headers containing `Q_OBJECT` or `Q_GADGET` — these **must** be here  |
| `UI_NONMOC_HEADERS`  | Qt-related headers with no `Q_OBJECT` (interfaces, coupling helpers)     |

Entries within each list are sorted alphabetically by path. Source files are registered in
the nearby `SNAP_CXX` and `UI_QT_CXX` blocks.

### Executables

| Target       | Entry point                              | Description                    |
| ------------ | ---------------------------------------- | ------------------------------ |
| `ITK-SNAP`   | `GUI/Qt/main.cxx`                        | The main GUI application       |
| `itksnap-wt` | `Utilities/Workspace/WorkspaceTool.cxx`  | Command-line workspace tool    |

## Coding style

- **C++17.** The minimum standard is set in the top-level `CMakeLists.txt`.
- **Use the project's ITK object conventions.** Logic and model classes derive from
  `itk::Object`, declare their boilerplate with `irisITKObjectMacro(ClassName, SuperClass)`,
  and are held in `SmartPtr<T>` (an alias for `itk::SmartPointer<T>`) and created with
  `irisNew<T>(...)`. Reserve raw `new` for Qt widgets, which Qt's parent-child ownership
  manages. Vector types come from `Common/IRISVectorTypes.h` (`Vector3d`, `Vector3ui`, …),
  not from raw arrays.
- **Route ITK events through the event bucket.** Connect ITK events to
  `onModelUpdate(EventBucket &)` via `LatentITKEventNotifier::connect`, then dispatch
  inside it with `b.HasEvent(SomeEvent(), source)`. Connecting an ITK event directly to a
  dedicated slot that ignores the bucket defeats the coalescing and causes duplicate
  handling.
- **Formatting** is defined by the `.clang-format` file at the repository root (written
  for clang-format 19.1.4). Format the lines you add or change. Do not reformat whole
  files you are otherwise not modifying — it buries the real change in noise and destroys
  `git blame`.
- **Match the surrounding code.** The codebase spans two decades and its conventions are
  not uniform; the CMake files in particular mix old-style (`SET`, `IF`) and modern
  commands. Consistency within a file beats global consistency.
- **Respect the layer boundaries.** No Qt headers in `Logic/` or `GUI/Model/`. If you find
  yourself wanting one, the design is telling you the state belongs in a model.
- **Portability.** The project builds with Clang, GCC, and MSVC. GCC and MSVC reject
  constructs Clang accepts, so a change that compiles on macOS can still fail CI —
  streaming a `std::string` into `qDebug()` and relying on transitive Qt includes are two
  recurring examples.

## Further reading

Also in this directory:

- [MemoryManagement.md](MemoryManagement.md) — owning vs. non-owning pointer patterns.
- [MemoryLeakTestingMacOS.md](MemoryLeakTestingMacOS.md) — running `leaks` against a build
  on macOS, and the expected baseline.
- [RemoteURLs.md](RemoteURLs.md) — the remote-URL loading scheme.

Elsewhere in the repository:

- [`Documentation/DesignNotes/gui_design.txt`](../DesignNotes/gui_design.txt) — original
  design notes for the GUI architecture.
- [`ReleaseNotes.md`](../../ReleaseNotes.md) — what changed in each release.

Online:

- [Building ITK-SNAP](http://www.itksnap.org/pmwiki/pmwiki.php?n=Documentation.BuildingITK-SNAP)
  — per-platform dependency instructions.
- [Mailing lists](http://www.itksnap.org/pmwiki/pmwiki.php?n=MailingLists) — the users'
  and developers' lists.
