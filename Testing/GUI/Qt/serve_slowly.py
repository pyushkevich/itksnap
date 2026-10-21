#!/usr/bin/env python3
"""Run a command while serving a directory over HTTP, slowly.

Usage: serve_slowly.py <directory> <port> <seconds> [--require-query Q]
                        -- <command> [args...]

Each file is sent in chunks spread over the given number of seconds, so that
a GUI test can check what ITK-SNAP shows while a download is in progress.
With --require-query, requests whose query string is not exactly Q are
refused (403), so a test can check that a URL reaches the server unchanged.
The server listens on 127.0.0.1 only. Exits with the command's exit code.
"""
import http.server
import os
import subprocess
import sys
import threading
import time
import urllib.parse

N_CHUNKS = 20


def make_handler(directory, seconds, require_query):
    class SlowHandler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            path, _, query = self.path.partition('?')
            if require_query is not None and query != require_query:
                self.send_error(403, 'Expected query ' + require_query)
                return
            name = os.path.basename(urllib.parse.unquote(path))
            path = os.path.join(directory, name)
            if not name or not os.path.isfile(path):
                self.send_error(404)
                return
            with open(path, 'rb') as f:
                data = f.read()
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            step = (len(data) + N_CHUNKS - 1) // N_CHUNKS
            for i in range(0, len(data), step):
                self.wfile.write(data[i:i + step])
                self.wfile.flush()
                time.sleep(seconds / N_CHUNKS)

        def log_message(self, fmt, *args):
            sys.stderr.write('serve_slowly: ' + (fmt % args) + '\n')

    return SlowHandler


def main():
    args = sys.argv[1:]
    require_query = None
    if len(args) > 4 and args[3] == '--require-query':
        require_query = args[4]
        del args[3:5]
    if len(args) < 5 or args[3] != '--':
        sys.exit(__doc__)
    directory, port, seconds = args[0], int(args[1]), float(args[2])
    command = args[4:]

    server = http.server.ThreadingHTTPServer(
        ('127.0.0.1', port), make_handler(directory, seconds, require_query))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        rc = subprocess.call(command)
    finally:
        server.shutdown()
    sys.exit(rc)


if __name__ == '__main__':
    main()
