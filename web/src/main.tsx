import { useCallback, useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { MenuBar } from './MenuBar';
import { FooterBar } from './FooterBar';
import { ContextMenu } from './ContextMenu';
import { setChromeMetrics } from './bridge';
import './styles.css';

function Shell() {
  // The canvas Emscripten/GLFW draws into. Created once and never replaced —
  // the same id (main-canvas) and tabindex the existing bridge expects, so
  // emscripten_glfw_set_next_window_canvas_selector and the IME/paste listeners
  // keep working unchanged.
  const canvasRef = useRef<HTMLCanvasElement>(null);

  // Chrome pixel heights, reported back to C++ so the ImGui document window is
  // positioned between the menu and footer.
  const [menuH, setMenuH] = useState(0);
  const [footerH, setFooterH] = useState(0);

  const reportMetrics = useCallback(
    (m: number, f: number) => setChromeMetrics(m, f),
    [],
  );
  useEffect(() => {
    reportMetrics(menuH, footerH);
  }, [menuH, footerH, reportMetrics]);

  // Set up window.Module with the canvas BEFORE stem.js (loaded in index.html
  // after this module) runs. Also mirror the old shell's shortcut blocking.
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    // Emscripten reads Module.canvas at startup; expose it early.
    window.Module = { ...(window.Module ?? {}), canvas };

    const onKey = (e: KeyboardEvent) => {
      if (e.metaKey || e.ctrlKey) {
        const k = e.key.toLowerCase();
        if (k === 'n' || k === 't' || k === 'w' || k === 'r' || k === 'p') {
          e.preventDefault();
        }
      }
    };
    window.addEventListener('keydown', onKey, { capture: true });
    return () => window.removeEventListener('keydown', onKey, true);
  }, []);

  return (
    <div className="mogan-shell">
      <div className="mogan-menubar-host">
        <MenuBar onHeight={setMenuH} />
      </div>
      <div className="mogan-canvas-host">
        <canvas
          id="main-canvas"
          ref={canvasRef}
          tabIndex={1}
          onContextMenu={(e) => e.preventDefault()}
        />
      </div>
      <div className="mogan-footer-host">
        <FooterBar onHeight={setFooterH} />
      </div>
      <ContextMenu />
    </div>
  );
}

const rootEl = document.getElementById('root');
if (rootEl) {
  createRoot(rootEl).render(<Shell />);
}
