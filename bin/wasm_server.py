from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import sys
import os

root = os.path.abspath(sys.argv[1])

html = os.path.join(root, "stem.html")
js = os.path.join(root, "stem.js")

#if not os.path.exists(html) and os.path.exists(js):
with open(html, "w", encoding="utf-8") as f:
    f.write("""<!doctype html>
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>stem</title>
  <style>
    html, body {
      margin: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
      background-color: #9f9f9f;
    }
    #main-canvas {
      display: block;
      width: 100%;
      height: 100%;
      outline: none;
    }
  </style>
</head>
<body>
  <canvas id="main-canvas" tabindex="1" oncontextmenu="event.preventDefault()"></canvas>
  <!-- tabindex="1" 允许 main-canvas 被获得 focus -->
  <script>
    var Module = {
      canvas: document.getElementById("main-canvas")
    };

    // 阻止浏览器在应用内触发新建窗口/标签页/打印等快捷键。
    // 使用 capture 阶段确保在浏览器默认处理之前拦截。
    window.addEventListener("keydown", function (e) {
      if (e.metaKey || e.ctrlKey) {
        var key = e.key.toLowerCase();
        if (key === "n" || key === "t" || key === "w" || key === "r" || key === "p") {
          e.preventDefault();
        }
      }
    }, { capture: true });
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