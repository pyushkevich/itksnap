# Remote URLs in ITK-SNAP

ITK-SNAP can open images and workspaces directly from remote sources by
passing a URL wherever a local file path is accepted. This document describes
the supported URL schemes, how authentication works for each, and the several
ways a URL can be handed to ITK-SNAP.

---

## The `itksnap-*` scheme prefix

Operating systems require a distinct, registered URL scheme to route a link
to the correct application. ITK-SNAP registers a family of `itksnap-*`
schemes for this purpose (e.g. `itksnap-fw://`, `itksnap-https://`). When
ITK-SNAP receives such a URL — whether from the OS handler or the `--url`
command-line flag — it strips the `itksnap-` prefix before processing:

```
itksnap-fw://example.flywheel.io/…  →  fw://example.flywheel.io/…
itksnap-https://example.org/…    →  https://example.org/…
```

The registered schemes are:

| Registered scheme | Underlying scheme |
|-------------------|-------------------|
| `itksnap-scp`     | `scp`             |
| `itksnap-sftp`    | `sftp`            |
| `itksnap-http`    | `http`            |
| `itksnap-https`   | `https`           |
| `itksnap-fw`      | `fw`              |

When constructing a URL to embed in a web page or share with a user, prefer
the `itksnap-*` form so the OS can open it directly.

---

## Supported URL types

### SSH / SFTP — `scp://` and `sftp://`

Downloads a single file over SSH using the SFTP subsystem.

```
scp://[user@]host[:port]/absolute/path/to/file.nii.gz
sftp://[user@]host[:port]/absolute/path/to/file.nii.gz
```

- `user` is optional; if omitted ITK-SNAP prompts for a username and password.
- `port` is optional; defaults to 22.
- The path must be absolute (starts with `/`).

**Authentication**: ITK-SNAP attempts public-key authentication first (using
keys in `~/.ssh`). If that fails it prompts for a password or key passphrase
via a dialog. Credentials are not stored between sessions.

**Example**:
```
scp://jdoe@imaging.hospital.org/data/study42/brain.nii.gz
sftp://jdoe@myserver.edu:2222/mnt/nfs/scans/sub-01_T1w.nii.gz
```

---

### HTTP / HTTPS — `http://` and `https://`

Downloads a file over plain HTTP or HTTPS. No authentication is performed;
the server must allow unauthenticated access.

```
http://example.org/path/to/file.nii.gz
https://example.org/path/to/file.nii.gz
```

**Caching**: The downloaded file is stored in the ITK-SNAP cache directory.
On subsequent opens of the same URL, a conditional `GET` with
`If-None-Match` / `If-Modified-Since` headers is sent; if the server returns
`304 Not Modified` the cached copy is used without re-downloading.

**Example**:
```
https://raw.githubusercontent.com/example/atlas/main/brain_atlas.nii.gz
```

---

### Flywheel — `fw://`

Downloads a file from a [Flywheel](https://flywheel.io) instance. Two URL
forms are supported.

#### Direct (ID-based)

Use this form when you already know the Flywheel container ID. It is the
most efficient form and is what ITK-SNAP workspaces store internally.

```
fw://<server>/acquisitions/<container-id>/files/<filename>
fw://<server>/analyses/<container-id>/files/<filename>
```

- `acquisitions` refers to an acquisition container.
- `analyses` refers to an analysis (gear output) container.

**Example**:
```
fw://example.flywheel.io/acquisitions/64a1c3f2e4b07d0012abcdef/files/brain.nii.gz
fw://example.flywheel.io/analyses/69ebae9505fb3e3183ff7935/files/output.nii.gz
```

#### Label-based (`find/`)

Use this form when constructing a URL from human-readable labels. ITK-SNAP
resolves the hierarchy group → project → subject → session → container by
label, then downloads the file. Label→ID resolutions are cached for the
lifetime of the application to avoid redundant API calls when loading a
workspace with many files from the same session.

```
fw://<server>/find/<group>/<project>/<subject>/<session>/acquisitions/<label>/files/<filename>
fw://<server>/find/<group>/<project>/<subject>/<session>/analyses/<label>/files/<filename>
```

**Example**:
```
fw://example.flywheel.io/find/pennbrain/StudyABC/sub-01/ses-01/acquisitions/T1w/files/brain.nii.gz
```

**Authentication**: ITK-SNAP looks for an API key in `~/.fw/config.yml`
(written by the Flywheel CLI — run `fw login` to populate it). If the file is
absent or no key matches the target server, a dialog prompts for the key. The
key may be in `host:secret` or bare `secret` format; both are accepted.
API keys are cached in memory for the session; a `401` response clears the
cache and shows an error asking the user to retry.

---

## Opening URLs

### Command-line flags

| Flag | Purpose |
|------|---------|
| `-g <url>` | Open as an image layer (main image or overlay) |
| `-w <url>` | Open as an ITK-SNAP workspace (`.itksnap` file) |
| `--url <url>` | Auto-detect: workspace if the path ends in `.itksnap`, image otherwise |

`--url` is the flag used by the OS URL-scheme handler; it also supports
ITK-SNAP's single-instance forwarding (image URLs are sent to an already-running
instance via IPC; workspace URLs always open a new window).

```sh
# Open a remote image as the main layer
ITK-SNAP -g "fw://example.flywheel.io/acquisitions/64a1c3f2e4b07d0012abcdef/files/brain.nii.gz"

# Open a remote workspace
ITK-SNAP -w "https://example.org/studies/study42.itksnap"

# Let ITK-SNAP decide (used by OS handler)
ITK-SNAP --url "itksnap-fw://example.flywheel.io/find/pennbrain/Study/sub-01/ses-01/acquisitions/T1w/files/brain.nii.gz"
```

### macOS — `open`

```sh
open "itksnap-fw://example.flywheel.io/acquisitions/64a1c3f2e4b07d0012abcdef/files/brain.nii.gz"
open "itksnap-https://example.org/studies/study42.itksnap"
```

### Windows — `start`

```bat
start "" "itksnap-fw://example.flywheel.io/acquisitions/64a1c3f2e4b07d0012abcdef/files/brain.nii.gz"
start "" "itksnap-https://example.org/studies/study42.itksnap"
```

### Browser / web page

Any hyperlink using an `itksnap-*://` scheme will trigger the OS handler:

```html
<a href="itksnap-fw://example.flywheel.io/acquisitions/64a1c3f2e4b07d0012abcdef/files/brain.nii.gz">
  Open in ITK-SNAP
</a>
```

The browser will show a confirmation dialog before launching ITK-SNAP. The
`itksnap-*` schemes are registered during installation on both macOS
(via `Info.plist`) and Windows (via NSIS registry entries under `HKCR`).

---

## Caching

All downloaded files are cached in the ITK-SNAP application data directory.
The cache avoids re-downloading unchanged files:

| Scheme | Cache key | Freshness check |
|--------|-----------|-----------------|
| `scp` / `sftp` | URL | Remote file size + mtime via SFTP `stat` |
| `http` / `https` | URL | HTTP `ETag` / `Last-Modified` conditional GET |
| `fw` | Original `fw://` URL | HTTP `ETag` / `Last-Modified` on the S3 download |
