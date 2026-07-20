from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import sys
import os

root = os.path.abspath(sys.argv[1])

html = os.path.join(root, "stem.html")
js = os.path.join(root, "stem.js")

class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=root, **kwargs)

    def end_headers(self):
        self.send_header(
            "Cross-Origin-Opener-Policy",
            "same-origin"
        )
        self.send_header(
            "Cross-Origin-Embedder-Policy",
            "require-corp"
        )
        super().end_headers()

server = ThreadingHTTPServer(
    ("127.0.0.1", 8000),
    Handler
)

print("Serving on http://127.0.0.1:8000/stem.html")

server.serve_forever()