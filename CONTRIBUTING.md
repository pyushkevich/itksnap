# Contributing to ITK-SNAP

Thank you for your interest in improving ITK-SNAP. ITK-SNAP is developed openly on
GitHub, and contributions of all kinds — bug reports, documentation, test cases, and
code — are welcome.

This document describes how to contribute. For who makes decisions and how the project
is run, see [GOVERNANCE.md](GOVERNANCE.md). All participants are expected to follow our
[Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to contribute

**Ask a question.** Usage questions are best asked on the
[ITK-SNAP Users' mailing list](http://www.itksnap.org/pmwiki/pmwiki.php?n=MailingLists),
not the issue tracker.

**Report a bug.** Open an issue at
<https://github.com/pyushkevich/itksnap/issues>. A good report includes:

- the ITK-SNAP version (**Help → About**) and your operating system;
- what you did, what you expected, and what happened instead;
- a minimal image or workspace that reproduces the problem, if you can share one.

**Suggest a feature.** Open an issue describing the problem you are trying to solve
before writing code. ITK-SNAP is used by a broad community across many imaging domains,
and a short discussion up front usually saves a lot of rework.

**Improve documentation.** Corrections to the developer documentation under
[`Documentation/Developer/`](Documentation/Developer/) are welcome and are reviewed the
same way as code.

**Contribute code.** See the workflow below.

## Before you start on code

- **Open an issue first for anything substantial** — new features, dependency changes,
  or changes that alter existing behavior. Small, self-contained bug fixes can go
  straight to a pull request.
- **Check that you can build and test the project.** Start with the
  [Developer Guide](Documentation/Developer/DeveloperGuide.md), which covers
  dependencies, configuring the build, and running the test suite.

## Development workflow

ITK-SNAP uses a fork-and-pull-request model.

1. **Fork** <https://github.com/pyushkevich/itksnap> and clone your fork
   **recursively** — ITK-SNAP has git submodules:

   ```
   git clone --recursive https://github.com/<your-username>/itksnap.git
   ```

2. **Create a topic branch** off `master` with a descriptive name:

   ```
   git checkout -b fix-polygon-tool-crash master
   ```

3. **Make your changes**, keeping the branch focused on one thing. Unrelated cleanups
   in the same branch make review harder and are usually asked to be split out.

4. **Build and test locally** before pushing (see the Developer Guide). At a minimum,
   the project should build without new warnings on your platform and `ctest` should
   show no new failures.

5. **Push** to your fork and **open a pull request** against `master`.

6. **Respond to review.** Push additional commits to the same branch; there is no need
   to force-push unless you are asked to.

## Commit messages

ITK-SNAP follows the commit-message convention used across the ITK/VTK/3D Slicer
ecosystem. Prefix the subject line with the kind of change:

| Prefix   | Meaning                                                     |
| -------- | ----------------------------------------------------------- |
| `BUG:`   | Fix for a runtime defect                                     |
| `ENH:`   | New functionality or an improvement to existing functionality |
| `DOC:`   | Documentation only                                           |
| `COMP:`  | Fix for a compiler or build error                            |
| `PERF:`  | Performance improvement                                      |
| `STYLE:` | Formatting or cosmetic change, no functional effect          |
| `WIP:`   | Work in progress, not ready to merge                         |

Guidelines:

- Keep the subject line under 72 characters and write it in the imperative mood
  ("Fix crash when closing a workspace", not "Fixed" or "Fixes").
- Use the body to explain **what** the change does and **why**, not how — the diff
  already shows how.
- Reference the issue it addresses (`Fixes #123`) when there is one.

Example:

```
BUG: Prevent crash when closing a workspace during mesh update

Closing a workspace while a background mesh build was running left the
Generic3DModel holding a dangling pointer to the destroyed image data.
Wait for the mesh worker to finish before tearing down the model.

Fixes #456
```

## What a pull request should contain

- **A clear description** of the problem and the approach taken.
- **Tests** for new functionality or for a bug fix, where the change is testable.
  Test data lives in `Testing/TestData/`; GUI tests are driven by scripts under
  `GUI/Qt/Resources/Scripts/`.
- **No unrelated changes** — no reformatting of files you did not otherwise touch.
- **Consistent formatting.** The repository has a `.clang-format` at its root; format
  the lines you add or change. Please do not reformat whole existing files, as this
  obscures history.
- **Backward compatibility.** ITK-SNAP reads workspaces and images written by older
  versions; changes to file formats, the workspace schema, or the command-line
  interface need explicit discussion.

## Continuous integration

Every push and pull request is built by GitHub Actions on Linux, macOS (Intel and Apple
Silicon), and Windows, and the test suite is run. A pull request is expected to be green
before it is merged. If CI fails for a reason you believe is unrelated to your change,
say so in the pull request rather than ignoring it.

## Review and merging

A maintainer will review your pull request. Because ITK-SNAP is maintained by a small
team alongside other research commitments, please allow some time for a first response,
and feel free to leave a polite reminder on the pull request if a week passes with no
reply.

Pull requests are merged by a maintainer once the review is resolved and CI is passing.
See [GOVERNANCE.md](GOVERNANCE.md) for who the maintainers are.

## Licensing

ITK-SNAP is distributed under the **GNU General Public License, version 3** (see
[`COPYING`](COPYING)). By submitting a contribution you agree that it may be distributed
under that license. Please do not include code you do not have the right to contribute,
or code taken from a project under an incompatible license.
