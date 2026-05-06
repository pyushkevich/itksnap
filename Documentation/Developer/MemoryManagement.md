# Memory Management Best Practices for ITK-SNAP

This document captures patterns that have caused memory leaks in ITK-SNAP and
the correct idioms to use instead. See also `MemoryLeakTestingMacOS.md` for
how to detect leaks during development.

---

## ITK `SmartPtr`: Owning vs. Non-Owning Back-Pointers

When two objects have a parent-child relationship, only the **parent should
hold a `SmartPtr` to the child**. The child should refer back to the parent
via a **raw pointer**:

```cpp
// Parent owns child:
SmartPtr<Child> m_Child;

// Child refers back — raw pointer, NOT SmartPtr:
Parent *m_Parent = nullptr;
```

A bidirectional `SmartPtr` cycle is the most common cause of
reference-counted leaks in ITK-based code. The canonical pattern in ITK-SNAP
is: the object that logically "contains" another holds a `SmartPtr`; the
contained object may hold a raw pointer back to its container, which is valid
for the container's lifetime.

**Known examples of this fix:**

| Owning side (`SmartPtr`) | Non-owning side (raw pointer) |
|---|---|
| `GenericImageData::m_MeshLayers` | `ImageMeshLayers::m_ImageData` |
| `WrapperBase::m_UserDataMap` (holds models) | `AbstractLayerTableRowModel::m_Layer` |

When using a raw back-pointer, guard every dereference against `nullptr` and
register for the source's `itk::DeleteEvent()` to null the pointer when the
referenced object is destroyed:

```cpp
// In Initialize():
Rebroadcast(m_Parent, itk::DeleteEvent(), ModelUpdateEvent());

// In OnUpdate():
if (m_EventBucket->HasEvent(itk::DeleteEvent(), m_Parent))
    m_Parent = nullptr;
```

---

## VTK Object Ownership (`vtkSmartPointer`)

Always initialise `vtkSmartPointer` members using `vtkSmartPointer<T>::New()`,
**never** the raw `T::New()`. `T::New()` returns a pointer with ref count = 1;
assigning it to a `vtkSmartPointer` calls `Register()`, raising the count to
2. When the smart pointer is destroyed the count drops to 1 — never to 0 — so
the object leaks permanently.

```cpp
// Correct — ref count stays at 1:
m_Actor = vtkSmartPointer<vtkActor>::New();

// Wrong — ref count is 2 from the start, object leaks:
m_Actor = vtkActor::New();
```

For factory methods annotated `VTK_NEWINSTANCE` (such as
`vtkCellArray::NewIterator()`), use `vtk::TakeSmartPointer()` to absorb the
returned owning raw pointer without an extra `Register()` call:

```cpp
// Correct for VTK_NEWINSTANCE return values:
auto it = vtk::TakeSmartPointer(cellArray->NewIterator());

// Wrong — same double-ref-count leak:
vtkSmartPointer<vtkCellArrayIterator> it = cellArray->NewIterator();
```

---

## Matching `new` with `delete` for Plain C++ Members

For plain C++ heap members (not ITK or VTK objects), every constructor that
calls `new` must have a corresponding `delete` in the destructor. Prefer
`std::unique_ptr<T>` to make ownership automatic and exception-safe:

```cpp
// Preferred — ownership is automatic:
std::unique_ptr<LayerHistogramPlotAssembly> m_HistogramAssembly;
// constructor: m_HistogramAssembly = std::make_unique<LayerHistogramPlotAssembly>();
// no destructor needed

// Acceptable — but requires explicit destructor:
LayerHistogramPlotAssembly *m_HistogramAssembly;
// constructor: m_HistogramAssembly = new LayerHistogramPlotAssembly();
// destructor:  delete m_HistogramAssembly;
```

When a class has a forward-declared type as a raw pointer member and the
destructor needs to `delete` it, the destructor body **must** be defined in
the `.cxx` file (where the full type is included), not inline in the header.
Deleting an incomplete type is undefined behaviour and is silently accepted by
some compilers.

---

## `Rebroadcaster::Rebroadcast()` — Call-Once Semantics

`Rebroadcaster::Rebroadcast()` (and the `AbstractModel::Rebroadcast()` wrapper)
should be treated as a **one-time setup call** per source-target-event triple.
Duplicate calls are silently ignored (deduplication was added in commit
`c0d8f4eb`), but the intent is that `Rebroadcast()` is called once, typically
in a constructor or a first-time initialization guard.

If a model's `Initialize()` method may be called multiple times (e.g. when
reloading a layer), verify that it either:

1. Is only called once (preferred), or
2. Clears existing connections before re-establishing them.

---

## Canary Tests for Leak Regressions

The two best canary tests for catching regressions in core model and renderer
setup are `PreferencesDialog` and `RandomForestBailOut`. They exercise full
GUI initialisation with minimal test-specific logic. A clean build should
report roughly **500–600 leaks / ~84 KB**, all attributable to Qt framework
ROOT CYCLEs (not addressable from ITK-SNAP code) and residual
`Rebroadcaster::Association` entries that require full application teardown to
release.

Any significant increase above this baseline indicates a new leak. See
`MemoryLeakTestingMacOS.md` for how to run these tests.
