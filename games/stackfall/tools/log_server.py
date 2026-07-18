#!/usr/bin/env python3
import datetime
import http.server
import socketserver
import sys
import urllib.parse


class LogHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        params = urllib.parse.parse_qs(parsed.query)
        message = params.get("m", [""])[0]
        source = params.get("s", ["tv"])[0]
        timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        if parsed.path == "/log":
            print(f"{timestamp} [{source}] {message}", flush=True)
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-store")
        self.end_headers()

    def log_message(self, format, *args):
        return


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
    with socketserver.TCPServer(("0.0.0.0", port), LogHandler) as server:
        print(f"listening on http://0.0.0.0:{port}/log", flush=True)
        server.serve_forever()


if __name__ == "__main__":
    main()
