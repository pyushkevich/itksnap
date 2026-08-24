# ITK-SNAP Governance

This document describes how the ITK-SNAP project is currently run: who makes decisions,
how contributions are accepted, and how releases are made. It describes existing
practice rather than aspiration, and it will be revised as the project's governance
develops.

## Principles

ITK-SNAP is free and open-source software, distributed under the
[GNU General Public License, version 3](COPYING). Development happens in the open at
<https://github.com/pyushkevich/itksnap>: the full source, issue tracker, and commit
history are public, and anyone may participate by opening issues or pull requests.

## Roles

**Users** use ITK-SNAP and participate by asking questions on the
[mailing lists](http://www.itksnap.org/pmwiki/pmwiki.php?n=MailingLists) and reporting
bugs. Users are the project's most important source of feedback.

**Contributors** submit changes — code, tests, or documentation — through pull requests.
Anyone can become a contributor; see [CONTRIBUTING.md](CONTRIBUTING.md). Contributors do
not have commit access to the main repository and work from forks.

**Maintainers** review and merge pull requests, triage issues, and have commit access to
the main repository. Maintainers are responsible for the technical direction and quality
of the code they merge.

**The lead maintainer** has final responsibility for the project: overall technical
direction, resolving disagreements that maintainers cannot settle among themselves,
deciding release contents and timing, and administering the project's repositories,
website, and distribution channels.

## Current maintainers

| Name                 | GitHub          | Role            |
| -------------------- | --------------- | --------------- |
| Paul A. Yushkevich   | @pyushkevich    | Lead maintainer |
| Jilei Hao            | @jilei-hao      | Maintainer      |

ITK-SNAP is the product of more than twenty years of work by many researchers, engineers,
and students. Contributors past and present are acknowledged on the
[Credits page](http://itksnap.org/credits.php).

## How decisions are made

Most decisions are made informally and in public, in the issue or pull request where the
work is being discussed. Where there is disagreement, the project seeks consensus among
the people doing the work; if consensus is not reached, the lead maintainer decides.

Changes that affect users beyond a single bug fix — new features, changes to file
formats or the workspace schema, changes to the command-line interface, new external
dependencies, or raising the minimum version of an existing dependency — should be
raised in an issue before implementation, so that the discussion is public and on the
record.

## Contributions and merging

Contributions from outside the maintainer group arrive as pull requests and are reviewed
by a maintainer before being merged. Maintainers may commit routine work directly, and
request review from each other for changes that are large, risky, or outside their
usual area.

Every push and pull request is built and tested on Linux, macOS, and Windows by GitHub
Actions. Changes are expected to be green in CI before they are merged.

## Releases

ITK-SNAP uses `MAJOR.MINOR.PATCH` version numbers. The lead maintainer decides what goes
into a release and when it ships, cuts the release, and publishes signed binary
installers through the project's established distribution channels
([itksnap.org](http://www.itksnap.org/download/snap) and SourceForge). Notable changes
are recorded in [`ReleaseNotes.md`](ReleaseNotes.md).

## Becoming a maintainer

There is no formal application process. Maintainers are invited by the existing
maintainers, on the basis of a sustained record of good contributions to the project and
of helpful participation in review and discussion. Someone who wants to work toward this
should start by contributing regularly and reviewing others' pull requests.

## Limits of this document, and how it changes

ITK-SNAP currently follows a **lead-maintainer model**, supported by a small core team.
The project does not yet have a broader formal governance structure — no steering
committee, no voting procedure, and no formal succession plan. How best to broaden
governance, and to reduce the project's reliance on a single maintainer, is under active
discussion; models such as the Insight Software Consortium are being considered.

This document is versioned with the source code. Proposed changes should be made as pull
requests and are approved by the lead maintainer.
