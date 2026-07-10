from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import sys
import os

root = os.path.abspath(sys.argv[1])

html = os.path.join(root, "stem.html")
js = os.path.join(root, "stem.js")

#if not os.path.exists(html) and os.path.exists(js):
with open(html, "w", encoding="utf-8") as f:
    f.write("""<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>stem</title>
  <style>
    html, body, canvas {
      margin: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
    }
  </style>
</head>
<body>
  <canvas id="canvas"></canvas>

  <script>
    var Module = {
      canvas: document.getElementById('canvas')
    };
  </script>

  <script src="stem.js"></script>
</body>
</html>
""")

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