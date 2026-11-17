# Remote URLs in ITK-SNAP

ITK-SNAP can open images and workspaces directly from remote sources by
passing a URL wherever a local file path is accepted, and can save image
layers back to SSH and Flywheel URLs. This document describes the supported
URL schemes, how authentication works for each, the several ways a URL can be
handed to ITK-SNAP, and how to test the remote code locally.

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

The environment variable `ITKSNAP_SSH_DIR` replaces `~/.ssh` as the folder
where keys, `known_hosts` and `config` are read. The tests use it so that they
never touch the user's own SSH files.

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
fw://<server>/sessions/<container-id>/files/<filename>
fw://<server>/subjects/<container-id>/files/<filename>
fw://<server>/projects/<container-id>/files/<filename>
```

- `acquisitions` refers to an acquisition container.
- `analyses` refers to an analysis (gear output) container.
- `sessions` refers to a session (visit) container.
- `subjects` refers to a subject container.
- `projects` refers to a project container.

**Example**:
```
fw://example.flywheel.io/acquisitions/64a1c3f2e4b07d0012abcdef/files/brain.nii.gz
fw://example.flywheel.io/analyses/69ebae9505fb3e3183ff7935/files/output.nii.gz
fw://example.flywheel.io/sessions/64b2d4a3f5c18e0023bcdef0/files/case.itksnap
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

The `find/` form still resolves only `acquisitions` or `analyses` at the leaf;
to open a file attached to a session, subject or project, use the direct form
above with that container's ID.

**Example**:
```
fw://example.flywheel.io/find/pennbrain/StudyABC/sub-01/ses-01/acquisitions/T1w/files/brain.nii.gz
```

**Authentication**: ITK-SNAP looks for an API key in `~/.fw/config.yml`
(written by the Flywheel CLI — run `fw login` to populate it). Only a key in
`host:secret` format whose host matches the target server is used; the
`default_profile` key is never sent to a different server. If the file is
absent or no key matches, a dialog prompts for the key, which may be in
`host:secret` or bare `secret` format.
API keys are cached in memory for the session; a `401` response clears the
cache and shows an error asking the user to retry.
An unrecognised `fw://` URL is now rejected before the API key is looked up, so
a malformed link no longer opens the key prompt.

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

## Saving to URLs

Image layers (main image, overlays and segmentations) can be saved to
`scp://`, `sftp://` and `fw://` URLs. `http(s)://` URLs are read-only.
Workspaces and meshes are always saved to local files.

- **Save** (e.g. Ctrl-S for the segmentation) on a layer opened from an
  `scp`, `sftp` or `fw` URL uploads it back to that URL, without a dialog.
  A layer opened from an `http(s)` URL opens the Save As wizard instead, with
  just the file name suggested.
- A new layer (e.g. a new segmentation) has no file name, so Save opens the
  wizard. If the main image came from an `scp`, `sftp` or `fw` URL, the
  wizard suggests `Untitled.nii.gz` next to it on the server, as it suggests
  the main image's folder for a local main image.
- **Save As** shows the layer's URL in the filename box, with the file name
  selected. Edit it to save under another name, or type a local path. The
  Browse button is disabled while the box holds a URL, since remote folders
  cannot be browsed. An `http(s)` URL is refused and Finish is disabled.
- The image is written to a temporary local folder and then uploaded. Formats
  that write two files (e.g. `.mhd` with `.raw`, `.hdr` with `.img`) upload
  both, to the same remote folder or container; the file the URL names goes
  last.
- The layer only counts as saved once the upload has succeeded. A failed or
  cancelled upload leaves it marked as modified.
- Saving to a URL drops that URL from the download cache, so reopening it
  downloads the new version.

**SSH / SFTP**: the file is written to a temporary name
(`.<name>.itksnap-upload`) in the target folder and then renamed over the
target, so a failed or cancelled upload never leaves a partial file under the
real name. If the server cannot rename over an existing file (SFTP version 3),
the old file is deleted first; if the rename then still fails, the error names
the temporary file that holds the upload.

**Flywheel**: the file is uploaded to the container the URL names, with
`POST /api/<type>/<id>/files`. Both the direct and the `find/` form work. An
upload under an existing file name makes a new version of that file in
Flywheel. A `401` response clears the cached API key; a `403` means the key's
user may not write to the container.

---

## Testing with local servers

`Testing/RemoteServer/remote_test_server.py` runs a command while serving
files over SFTP (with the `asyncssh` Python module), HTTP and a mock Flywheel
API, all on `127.0.0.1`. The `RemoteSaveTest_*`, `RemoteImageLoadTest_SFTPServer`,
`RemoteImageLoadTest_Flywheel*` and `RemoteSaveSFTP` (GUI) tests use it. They
need no internet access; without `asyncssh` (`pip install asyncssh`) they are
skipped. The script's docstring describes the folders it serves and the test
user and API key.

The server sets two environment variables for the command:

| Variable | Effect |
|----------|--------|
| `ITKSNAP_SSH_DIR` | Read SSH keys, `known_hosts` and `config` from this folder instead of `~/.ssh` |
| `ITKSNAP_FW_TEST_ALLOW_HTTP=1` | Use plain `http://` for `fw://` URLs, but only when the host is `localhost` or `127.0.0.1` |

The server also sets `HOME` to a folder holding a Flywheel CLI `config.yml`
with its API key.

---

## Caching

All downloaded files are cached in the ITK-SNAP application data directory.
The cache avoids re-downloading unchanged files:

| Scheme | Cache key | Freshness check |
|--------|-----------|-----------------|
| `scp` / `sftp` | URL | Remote file size + mtime via SFTP `stat` |
| `http` / `https` | URL | HTTP `ETag` / `Last-Modified` conditional GET |
| `fw` | Original `fw://` URL | HTTP `ETag` / `Last-Modified` on the S3 download |
