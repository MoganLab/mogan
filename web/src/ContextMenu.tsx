import { useEffect, useRef, useState } from 'react';
import type { MenuNode } from './types';
import { closePopup, subscribePopupClose, subscribePopupOpen } from './bridge';
import { MenuItems } from './MenuItems';

interface PopupState {
  tree: MenuNode[];
  x: number;
  y: number;
}

/**
 * Right-click context menu. The C++ side (im_activate_popup under WASM)
 * serializes the same popup widget tree the native ImGui build renders and
 * asks us to show it at a screen position. Reuses MenuItems for rendering.
 *
 * Closes on outside click or Esc, calling closePopup() so C++ deactivates the
 * active popup and resumes forwarding mouse events to the editor.
 */
export function ContextMenu() {
  const [popup, setPopup] = useState<PopupState | null>(null);
  const menuRef = useRef<HTMLUListElement>(null);

  useEffect(
    () =>
      subscribePopupOpen((tree, x, y) =>
        setPopup({ tree, x, y }),
      ),
    [],
  );
  useEffect(
    () =>
      subscribePopupClose(() => setPopup(null)),
    [],
  );

  useEffect(() => {
    if (!popup) return;
    const onPointerDown = (e: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
        closePopup();
        setPopup(null);
      }
    };
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        closePopup();
        setPopup(null);
      }
    };
    window.addEventListener('mousedown', onPointerDown, true);
    window.addEventListener('keydown', onKey, true);
    return () => {
      window.removeEventListener('mousedown', onPointerDown, true);
      window.removeEventListener('keydown', onKey, true);
    };
  }, [popup]);

  if (!popup) return null;

  // Clamp so the menu stays within the viewport (CSS alone can't know size).
  const left = Math.min(popup.x, window.innerWidth - 200);
  const top = Math.min(popup.y, window.innerHeight - 200);

  return (
    <ul
      ref={menuRef}
      className="mogan-menu-dropdown mogan-context-menu"
      role="menu"
      style={{ left, top }}
    >
      <MenuItems nodes={popup.tree} onInvoke={() => setPopup(null)} />
    </ul>
  );
}
