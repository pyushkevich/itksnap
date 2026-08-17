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

4. **Build and test locally** before pushing, if you can (see the Developer Guide).
   Checking that the project still builds on your platform and that `ctest` shows no new
   failures will usually save a review round-trip — but CI checks this too, so a pull
   request you could not fully test locally is still worth opening.

5. **Push** to your fork and **open a pull request** against `master`.

6. **Respond to review.** Push additional commits to the same branch; there is no need
   to force-push unless you are asked to.

## Commit messages

**These are recommendations, not requirements.** The maintainers are adopting this
convention themselves first, and will consider asking for it more firmly only once it has
proven worth the friction in day-to-day use. A clear, honest commit message that follows
none of the advice below is worth more than a well-formatted one that does not explain the
change — please do not let formatting rules stop you from contributing.

That said, the convention we are moving toward is the one used across the ITK/VTK/3D Slicer
ecosystem: prefix the subject line with the kind of change.

| Prefix   | Meaning                                                     |
| -------- | ----------------------------------------------------------- |
| `BUG:`   | Fix for a runtime defect                                     |
| `ENH:`   | New functionality or an improvement to existing functionality |
| `DOC:`   | Documentation only                                           |
| `COMP:`  | Fix for a compiler or build error                            |
| `PERF:`  | Performance improvement                                      |
| `STYLE:` | Formatting or cosmetic change, no functional effect          |
| `WIP:`   | Work in progress, not ready to merge                         |

Suggestions that tend to help:

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

## What makes a pull request easy to review

These are recommendations rather than a checklist you must satisfy. They describe what
tends to make a change quick to review and merge; none of them is a precondition for
opening a pull request, and a maintainer can help with any of them during review.

- **A clear description** of the problem and the approach taken. This is the one that
  helps most, and it costs the least.
- **Tests** for new functionality or for a bug fix, where the change is testable. Test data
  lives in `Testing/TestData/`; GUI test scripts live in `Testing/GUI/Qt/Scripts/`. If you
  are not sure how to test something, open the pull request anyway and ask.
- **No unrelated changes** — keeping reformatting out of a functional change makes the
  functional part visible.
- **Consistent formatting.** The repository has a `.clang-format` at its root; formatting
  the lines you add or change is helpful. Please avoid reformatting whole existing files,
  which obscures history and makes the real change hard to find.
- **Backward compatibility.** This is the one place we would genuinely rather hear from you
  first. ITK-SNAP opens workspaces and images written by much older versions, and users
  depend on that. If your change touches a file format, the workspace schema, or the
  command-line interface, please raise it in an issue before investing much time — not
  because the change is unwelcome, but because getting the compatibility story right early
  is much easier than reworking it later.

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
