import { useCallback, useEffect, useState } from 'react';
import type { MenuNode } from './types';
import { closePopup, subscribePopupClose, subscribePopupOpen } from './bridge';
import { MenuItems, useCanScrollClass } from './MenuItems';

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
 * Shares the menu bar's open semantics: it does NOT close when the pointer
 * leaves. It closes only on an explicit gesture — an enabled item firing its
 * command, Escape, or a mousedown outside the menu. That outside mousedown is
 * swallowed in the capture phase so it never reaches the editor canvas. Every
 * dismiss path also calls closePopup() so C++ deactivates the active popup and
 * resumes forwarding mouse events to the editor (its GLFW callbacks
 * early-return while a popup is active).
 */
export function ContextMenu() {
  const [popup, setPopup] = useState<PopupState | null>(null);
  const dd = useCanScrollClass<HTMLUListElement>([popup]);

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

  // C++ deactivates the active popup (resuming editor mouse dispatch) only
  // when closePopup() runs — so every dismiss path must call it, not just
  // hide the menu locally. Hide locally FIRST (sync setState) and defer the
  // ccall: closePopup's mogan_menu_close_popup runs synchronously and would
  // otherwise delay the dismiss repaint, making the popup feel stuck.
  const dismiss = useCallback(() => {
    setPopup(null);
    setTimeout(() => closePopup(), 0);
  }, []);

  useEffect(() => {
    if (!popup) return;
    const onPointerDown = (e: MouseEvent) => {
      // Window-level: a click inside any dropdown layer (including nested
      // flyouts, which aren't DOM-nested under this popup) is "inside" and
      // must not dismiss. Only a genuinely outside click closes — and it's
      // swallowed here so it never falls through to the canvas.
      const inside = (e.target as Element | null)?.closest?.(
        '.mogan-menu-dropdown',
      );
      if (!inside) {
        e.stopPropagation();
        dismiss();
      }
    };
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        dismiss();
      }
    };
    window.addEventListener('mousedown', onPointerDown, true);
    window.addEventListener('keydown', onKey, true);
    return () => {
      window.removeEventListener('mousedown', onPointerDown, true);
      window.removeEventListener('keydown', onKey, true);
    };
  }, [popup, dismiss]);

  if (!popup) return null;

  // Clamp so the menu stays within the viewport (CSS alone can't know size).
  const left = Math.min(popup.x, window.innerWidth - 200);
  const top = Math.min(popup.y, window.innerHeight - 200);

  return (
    <ul
      ref={dd.ref}
      className={'mogan-menu-dropdown mogan-context-menu' + dd.cls}
      role="menu"
      style={{ left, top }}
    >
      <MenuItems nodes={popup.tree} onInvoke={dismiss} />
    </ul>
  );
}
