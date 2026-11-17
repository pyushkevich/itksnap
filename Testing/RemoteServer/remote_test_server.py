#!/usr/bin/env python3
"""Run a command while serving files over SFTP, HTTP and a mock Flywheel API.

Usage: remote_test_server.py --root DIR [--data SRC]
                             [--sftp-port P] [--http-port P] [--fw-port P]
                             [--fw-key KEY] [--fw-config-key KEY]
                             [--sftp-write-delay SECONDS]
                             -- <command> [args...]
       remote_test_server.py --root DIR --self-test

The servers let ITK-SNAP's remote load and save code be tested without a real
SSH server or Flywheel instance. They listen on 127.0.0.1 only. DIR is
emptied and filled with:

  DIR/sftp/       the SFTP root: scp://testuser@127.0.0.1:P/<file> is DIR/sftp/<file>
  DIR/http/       files served at http://127.0.0.1:P/<file>
  DIR/fw/         the mock Flywheel containers (see FW_TREE below)
  DIR/ssh/        a client key, known_hosts and config for the SFTP server
  DIR/home/.fw/   a Flywheel CLI config.yml holding the API key

The files in SRC (--data) are copied to the SFTP root, the HTTP root and the
mock Flywheel acquisition. The command runs with ITKSNAP_SSH_DIR=DIR/ssh, so
that libssh reads keys and known_hosts from there and never from the user's
~/.ssh; HOME=DIR/home, so that the Flywheel key is found there; and
ITKSNAP_FW_TEST_ALLOW_HTTP=1, so that fw://127.0.0.1:P/... uses plain http.
SFTP accepts the client key or the password 'testpassword' for 'testuser'.

Exits with the command's exit code, or with 77 (which CTest can treat as a
skipped test) when the asyncssh module is not installed.
"""
import argparse
import asyncio
import email.parser
import email.policy
import email.utils
import hashlib
import http.server
import json
import os
import shutil
import sys
import threading
import urllib.parse
import uuid

SKIP_EXIT_CODE = 77
SSH_USER = 'testuser'
SSH_PASSWORD = 'testpassword'

# The mock Flywheel hierarchy. Labels resolve to IDs through the list
# endpoints that fw://host/find/... URLs use.
FW_GROUP = 'testgroup'
FW_TREE = {
    'projects':     [{'_id': 'p0001', 'label': 'TestProject', 'parent': FW_GROUP}],
    'subjects':     [{'_id': 's0001', 'label': 'sub-01', 'parent': 'p0001'}],
    'sessions':     [{'_id': 'e0001', 'label': 'ses-01', 'parent': 's0001'}],
    'acquisitions': [{'_id': 'a0001', 'label': 'T1w', 'parent': 'e0001'}],
    'analyses':     [{'_id': 'n0001', 'label': 'seg', 'parent': 'e0001'}],
}
FW_CONTAINER_TYPES = ('projects', 'subjects', 'sessions', 'acquisitions', 'analyses')
FW_CHILD_TYPE = {'groups': 'projects', 'projects': 'subjects',
                 'subjects': 'sessions'}


def log(msg):
    print('[remote_test_server] ' + msg, flush=True)


# ---------------------------------------------------------------------------
#  HTTP: static files, and the mock Flywheel API
# ---------------------------------------------------------------------------

def send_file(handler, path, head_only=False):
    """Send a file with ETag and Last-Modified, honouring If-None-Match."""
    if not os.path.isfile(path):
        handler.send_error(404)
        return
    st = os.stat(path)
    etag = '"%x-%x"' % (st.st_size, st.st_mtime_ns)
    if handler.headers.get('If-None-Match') == etag:
        handler.send_response(304)
        handler.send_header('ETag', etag)
        handler.end_headers()
        return
    handler.send_response(200)
    handler.send_header('Content-Length', str(st.st_size))
    handler.send_header('ETag', etag)
    handler.send_header('Last-Modified',
                        email.utils.formatdate(st.st_mtime, usegmt=True))
    handler.end_headers()
    if not head_only:
        with open(path, 'rb') as f:
            shutil.copyfileobj(f, handler.wfile)


def safe_join(root, *parts):
    """Join URL path parts under root, refusing anything that escapes it."""
    path = os.path.realpath(os.path.join(root, *parts))
    if path != root and not path.startswith(root + os.sep):
        return None
    return path


def make_static_handler(root):
    class StaticHandler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            rel = urllib.parse.unquote(urllib.parse.urlsplit(self.path).path)
            path = safe_join(root, rel.lstrip('/'))
            if path is None:
                self.send_error(403)
                return
            send_file(self, path)

        def log_message(self, fmt, *args):
            log('http: ' + fmt % args)

    return StaticHandler


def make_flywheel_handler(root, api_key):
    tickets = {}            # ticket -> file path
    lock = threading.Lock()

    class FlywheelHandler(http.server.BaseHTTPRequestHandler):
        def authorized(self):
            if self.headers.get('Authorization') == 'Bearer ' + api_key:
                return True
            self.send_json(401, {'message': 'Invalid API key'})
            return False

        def send_json(self, code, obj):
            body = json.dumps(obj).encode()
            self.send_response(code)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def split(self):
            url = urllib.parse.urlsplit(self.path)
            parts = [urllib.parse.unquote(p) for p in url.path.split('/') if p]
            return parts, urllib.parse.parse_qs(url.query, keep_blank_values=True)

        def do_GET(self):
            parts, query = self.split()
            if not parts or parts[0] != 'api':
                self.send_error(404)
                return
            parts = parts[1:]

            # /api/<type>/<id>/files/<name>: a ticket, or the file with one
            if len(parts) == 4 and parts[0] in FW_CONTAINER_TYPES and parts[2] == 'files':
                path = safe_join(root, parts[0], parts[1], parts[3])
                if path is None:
                    self.send_error(403)
                    return
                if 'ticket' in query and query['ticket'][0]:
                    with lock:
                        ok = tickets.pop(query['ticket'][0], None) == path
                    if not ok:
                        self.send_json(401, {'message': 'Invalid ticket'})
                        return
                    send_file(self, path)
                    return
                if not self.authorized():
                    return
                if not os.path.isfile(path):
                    self.send_json(404, {'message': 'File not found'})
                    return
                ticket = uuid.uuid4().hex
                with lock:
                    tickets[ticket] = path
                self.send_json(200, {'ticket': ticket})
                return

            # /api/<parent type>/<parent id>/<child type>: list the children
            if len(parts) == 3:
                ptype, pid, ctype = parts
                expected = FW_CHILD_TYPE.get(ptype)
                if ptype == 'sessions':
                    expected = ctype if ctype in ('acquisitions', 'analyses') else None
                if ctype != expected:
                    self.send_error(404)
                    return
                if not self.authorized():
                    return
                items = [{'_id': c['_id'], 'label': c['label']}
                         for c in FW_TREE[ctype] if c['parent'] == pid]
                self.send_json(200, items)
                return

            self.send_error(404)

        def do_POST(self):
            # /api/<type>/<id>/files: a multipart upload, part 'file'
            parts, _ = self.split()
            if not (len(parts) == 4 and parts[0] == 'api'
                    and parts[1] in FW_CONTAINER_TYPES and parts[3] == 'files'):
                self.send_error(404)
                return
            if not self.authorized():
                return
            ctype = self.headers.get('Content-Type', '')
            length = int(self.headers.get('Content-Length', '0'))
            body = self.rfile.read(length)
            if len(body) < length:
                # The client gave up (e.g. a cancelled upload): store nothing
                log('flywheel: upload aborted after %d of %d bytes' % (len(body), length))
                return
            msg = email.parser.BytesParser(policy=email.policy.HTTP).parsebytes(
                b'Content-Type: ' + ctype.encode() + b'\r\n\r\n' + body)
            saved = []
            for part in msg.iter_parts():
                if part.get_param('name', header='content-disposition') != 'file':
                    continue
                name = os.path.basename(part.get_filename() or '')
                path = safe_join(root, parts[1], parts[2], name) if name else None
                if path is None:
                    self.send_json(400, {'message': 'Bad file name'})
                    return
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path + '.part', 'wb') as f:
                    f.write(part.get_payload(decode=True))
                os.replace(path + '.part', path)
                saved.append({'name': name, 'size': os.path.getsize(path)})
            if not saved:
                self.send_json(400, {'message': "No 'file' part in the upload"})
                return
            log('flywheel: stored %s in %s/%s' % (
                ', '.join(s['name'] for s in saved), parts[1], parts[2]))
            self.send_json(200, saved)

        def log_message(self, fmt, *args):
            log('flywheel: ' + fmt % args)

    return FlywheelHandler


def start_http(port, handler):
    server = http.server.ThreadingHTTPServer(('127.0.0.1', port), handler)
    server.daemon_threads = True
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


# ---------------------------------------------------------------------------
#  SFTP
# ---------------------------------------------------------------------------

async def start_sftp(asyncssh, port, root, ssh_dir, write_delay):
    host_key = asyncssh.generate_private_key('ssh-ed25519')
    client_key = asyncssh.generate_private_key('ssh-ed25519')

    # The client side: a key that the server accepts, a known_hosts entry
    # for the server's key, and an empty config so ~/.ssh/config is not read
    os.makedirs(ssh_dir, exist_ok=True)
    client_key.write_private_key(os.path.join(ssh_dir, 'id_ed25519'))
    client_key.write_public_key(os.path.join(ssh_dir, 'id_ed25519.pub'))
    os.chmod(os.path.join(ssh_dir, 'id_ed25519'), 0o600)
    host_pub = host_key.export_public_key('openssh').decode().strip()
    with open(os.path.join(ssh_dir, 'known_hosts'), 'w') as f:
        for host in ('127.0.0.1', 'localhost'):
            f.write('[%s]:%d %s\n' % (host, port, host_pub))
    open(os.path.join(ssh_dir, 'config'), 'w').close()

    authorized = asyncssh.import_authorized_keys(
        client_key.export_public_key('openssh').decode())

    class Server(asyncssh.SSHServer):
        def begin_auth(self, username):
            return True

        def password_auth_supported(self):
            return True

        def validate_password(self, username, password):
            return username == SSH_USER and password == SSH_PASSWORD

    class SFTPServer(asyncssh.SFTPServer):
        def __init__(self, chan):
            super().__init__(chan, chroot=root)

        async def write(self, file_obj, offset, data):
            if write_delay > 0:
                await asyncio.sleep(write_delay)
            return super().write(file_obj, offset, data)

    return await asyncssh.listen(
        '127.0.0.1', port, server_host_keys=[host_key],
        authorized_client_keys=authorized, server_factory=Server,
        sftp_factory=SFTPServer, allow_scp=True)


# ---------------------------------------------------------------------------
#  Setup, self test and main
# ---------------------------------------------------------------------------

def prepare_root(root, data):
    if os.path.isdir(root):
        shutil.rmtree(root)
    dirs = {name: os.path.join(root, name) for name in ('sftp', 'http', 'fw', 'ssh', 'home')}
    for d in dirs.values():
        os.makedirs(d)
    for ctype in FW_CONTAINER_TYPES:
        for c in FW_TREE[ctype]:
            os.makedirs(os.path.join(dirs['fw'], ctype, c['_id']))
    if data:
        for name in sorted(os.listdir(data)):
            src = os.path.join(data, name)
            if os.path.isfile(src):
                for dest in (dirs['sftp'], dirs['http'],
                             os.path.join(dirs['fw'], 'acquisitions', 'a0001')):
                    shutil.copy2(src, dest)
    return dirs


def write_fw_config(home, fw_port, api_key):
    os.makedirs(os.path.join(home, '.fw'), exist_ok=True)
    with open(os.path.join(home, '.fw', 'config.yml'), 'w') as f:
        f.write('api_key: 127.0.0.1:%d:%s\n' % (fw_port, api_key))


async def self_test(asyncssh, args, dirs):
    """Exercise every server with Python clients."""
    import urllib.request
    payload = os.urandom(200000)
    digest = hashlib.sha256(payload).hexdigest()

    # SFTP: key login, put, get, posix-rename; password login
    async with asyncssh.connect(
            '127.0.0.1', args.sftp_port, username=SSH_USER,
            client_keys=[os.path.join(dirs['ssh'], 'id_ed25519')],
            known_hosts=os.path.join(dirs['ssh'], 'known_hosts')) as conn:
        async with conn.start_sftp_client() as sftp:
            async with sftp.open('/up.tmp', 'wb') as f:
                await f.write(payload)
            await sftp.posix_rename('/up.tmp', '/up.bin')
            async with sftp.open('/up.bin', 'rb') as f:
                assert hashlib.sha256(await f.read()).hexdigest() == digest
    assert open(os.path.join(dirs['sftp'], 'up.bin'), 'rb').read() == payload
    async with asyncssh.connect(
            '127.0.0.1', args.sftp_port, username=SSH_USER, password=SSH_PASSWORD,
            client_keys=None,
            known_hosts=os.path.join(dirs['ssh'], 'known_hosts')) as conn:
        async with conn.start_sftp_client() as sftp:
            assert await sftp.exists('/up.bin')
    log('self-test: SFTP ok')

    # HTTP: a file and a 304
    with open(os.path.join(dirs['http'], 'h.bin'), 'wb') as f:
        f.write(payload)
    base = 'http://127.0.0.1:%d/' % args.http_port
    with urllib.request.urlopen(base + 'h.bin') as r:
        assert r.read() == payload
        etag = r.headers['ETag']
    try:
        urllib.request.urlopen(urllib.request.Request(
            base + 'h.bin', headers={'If-None-Match': etag}))
        raise AssertionError('expected 304')
    except urllib.error.HTTPError as e:
        assert e.code == 304
    log('self-test: HTTP ok')

    # Flywheel: label lookup, upload, ticket download, bad key
    fw = 'http://127.0.0.1:%d/api/' % args.fw_port
    auth = {'Authorization': 'Bearer ' + args.fw_key}

    def get_json(url, headers=auth):
        with urllib.request.urlopen(urllib.request.Request(url, headers=headers)) as r:
            return json.loads(r.read())

    ids = {}
    for path, label in (('groups/%s/projects' % FW_GROUP, 'TestProject'),
                        ('projects/p0001/subjects', 'sub-01'),
                        ('subjects/s0001/sessions', 'ses-01'),
                        ('sessions/e0001/acquisitions', 'T1w')):
        ids[label] = [x['_id'] for x in get_json(fw + path) if x['label'] == label][0]
    assert ids['T1w'] == 'a0001'

    boundary = uuid.uuid4().hex
    body = (('--%s\r\nContent-Disposition: form-data; name="file"; '
             'filename="u.bin"\r\nContent-Type: application/octet-stream\r\n\r\n'
             % boundary).encode() + payload + ('\r\n--%s--\r\n' % boundary).encode())
    req = urllib.request.Request(
        fw + 'acquisitions/a0001/files', data=body, method='POST',
        headers=dict(auth, **{'Content-Type': 'multipart/form-data; boundary=' + boundary}))
    with urllib.request.urlopen(req) as r:
        assert json.loads(r.read())[0]['size'] == len(payload)
    ticket = get_json(fw + 'acquisitions/a0001/files/u.bin?ticket=')['ticket']
    with urllib.request.urlopen(fw + 'acquisitions/a0001/files/u.bin?ticket=' + ticket) as r:
        assert hashlib.sha256(r.read()).hexdigest() == digest
    try:
        get_json(fw + 'projects/p0001/subjects', headers={'Authorization': 'Bearer wrong'})
        raise AssertionError('expected 401')
    except urllib.error.HTTPError as e:
        assert e.code == 401
    log('self-test: Flywheel ok')


async def run(args):
    try:
        import asyncssh
    except ImportError:
        log('the asyncssh module is not installed (pip install asyncssh); skipping')
        return SKIP_EXIT_CODE

    root = os.path.realpath(args.root)
    dirs = prepare_root(root, args.data)
    write_fw_config(dirs['home'], args.fw_port, args.fw_config_key or args.fw_key)

    sftp = await start_sftp(asyncssh, args.sftp_port, dirs['sftp'], dirs['ssh'],
                            args.sftp_write_delay)
    servers = [start_http(args.http_port, make_static_handler(dirs['http'])),
               start_http(args.fw_port, make_flywheel_handler(dirs['fw'], args.fw_key))]
    log('serving %s: sftp %d, http %d, flywheel %d' % (
        root, args.sftp_port, args.http_port, args.fw_port))
    try:
        if args.self_test:
            await self_test(asyncssh, args, dirs)
            return 0
        env = dict(os.environ,
                   ITKSNAP_SSH_DIR=dirs['ssh'],
                   ITKSNAP_FW_TEST_ALLOW_HTTP='1',
                   HOME=dirs['home'])
        proc = await asyncio.create_subprocess_exec(*args.command, env=env)
        return await proc.wait()
    finally:
        sftp.close()
        for s in servers:
            s.shutdown()


def main():
    argv = sys.argv[1:]
    command = []
    if '--' in argv:
        command = argv[argv.index('--') + 1:]
        argv = argv[:argv.index('--')]
    p = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    p.add_argument('--root', required=True)
    p.add_argument('--data')
    p.add_argument('--sftp-port', type=int, default=8930)
    p.add_argument('--http-port', type=int, default=8931)
    p.add_argument('--fw-port', type=int, default=8932)
    p.add_argument('--fw-key', default='itksnaptestkey',
                   help='the API key that the mock Flywheel server accepts')
    p.add_argument('--fw-config-key',
                   help='the key written to the client\'s config.yml, if not --fw-key')
    p.add_argument('--sftp-write-delay', type=float, default=0.0,
                   help='seconds to wait on every SFTP write request')
    p.add_argument('--self-test', action='store_true')
    args = p.parse_args(argv)
    args.command = command
    if not args.self_test and not command:
        p.error('give a command after --, or --self-test')
    sys.exit(asyncio.run(run(args)))


if __name__ == '__main__':
    main()
