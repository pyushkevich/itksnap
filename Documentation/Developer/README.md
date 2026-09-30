# ITK-SNAP Developer Documentation

Documentation for people building and modifying ITK-SNAP. End-user documentation lives at
[itksnap.org](http://www.itksnap.org/pmwiki/pmwiki.php?n=Documentation.HomePage).

**Start here:**

- **[DeveloperGuide.md](DeveloperGuide.md)** — building from source, running the tests, a
  tour of the architecture, and where to make common kinds of change.

**Reference:**

- [MemoryManagement.md](MemoryManagement.md) — owning vs. non-owning pointer patterns, and
  the ownership mistakes that caused past leaks.
- [MemoryLeakTestingMacOS.md](MemoryLeakTestingMacOS.md) — running `leaks` against a local
  build on macOS, including the expected baseline for the canary tests.
- [RemoteURLs.md](RemoteURLs.md) — how ITK-SNAP loads images and workspaces from remote
  URLs.

**Design notes:**

- [../DesignNotes/gui_design.txt](../DesignNotes/gui_design.txt) — original notes on the
  GUI architecture.

**Contributing:**

- [CONTRIBUTING.md](../../CONTRIBUTING.md) — how to report bugs and submit pull requests.
- [GOVERNANCE.md](../../GOVERNANCE.md) — how the project is run and who maintains it.
- [CODE_OF_CONDUCT.md](../../CODE_OF_CONDUCT.md) — expectations for participation.
