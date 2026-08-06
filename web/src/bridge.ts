/**
 * Typed bridge between the React shell and the Emscripten/WASM module.
 *
 * Two directions of communication:
 *
 * 1. C++ → JS: the C++ side calls window.moganOnMenu / moganOnFooter /
 *    moganOnOpenPopup / moganOnClosePopup (registered here via the subscribe*
 *    helpers) to push serialized state. Those callbacks are installed by
 *    EM_JS hooks in src/Plugins/ImGui/im_react_bridge.cpp.
 *
 * 2. JS → C++: React invokes Module.ccall to run a menu command, close the
 *    context menu, or report chrome pixel heights back to C++ so the document
 *    canvas is laid out between the menu and footer.
 *
 * Before stem.js loads we must set window.Module so Emscripten picks up our
 * canvas element — done in main.tsx.
 */
import type { FooterState, MenuNode } from './types';

type MenuListener = (tree: MenuNode[]) => void;
type FooterListener = (state: FooterState) => void;
type PopupOpenListener = (tree: MenuNode[], x: number, y: number) => void;
type PopupCloseListener = () => void;
/** Delivers a lazily-expanded submenu's children: (submenu id, children). */
type SubmenuListener = (id: number, children: MenuNode[]) => void;

type CcallFn = (
  name: string,
  returnType: string | null,
  argTypes: string[],
  args: unknown[],
) => unknown;

type EmscriptenModule = {
  canvas?: HTMLElement;
  ccall?: CcallFn;
  cwrap?: (
    name: string,
    returnType: string | null,
    argTypes: string[],
  ) => (...args: unknown[]) => unknown;
  [key: string]: unknown;
};

declare global {
  interface Window {
    Module?: EmscriptenModule;
    moganOnMenu?: MenuListener;
    moganOnFooter?: FooterListener;
    moganOnOpenPopup?: PopupOpenListener;
    moganOnClosePopup?: PopupCloseListener;
    moganOnSubmenu?: SubmenuListener;
  }
}

/** Subscribe to the next full menu-tree replacement (main menu rebuilt). */
export function subscribeMenu(cb: MenuListener): () => void {
  window.moganOnMenu = cb;
  return () => {
    if (window.moganOnMenu === cb) window.moganOnMenu = undefined;
  };
}

/** Subscribe to footer (status bar) state updates. */
export function subscribeFooter(cb: FooterListener): () => void {
  window.moganOnFooter = cb;
  return () => {
    if (window.moganOnFooter === cb) window.moganOnFooter = undefined;
  };
}

/** Subscribe to right-click context menu open events. */
export function subscribePopupOpen(cb: PopupOpenListener): () => void {
  window.moganOnOpenPopup = cb;
  return () => {
    if (window.moganOnOpenPopup === cb) window.moganOnOpenPopup = undefined;
  };
}

/** Subscribe to context menu close events (C++ dismissed it). */
export function subscribePopupClose(cb: PopupCloseListener): () => void {
  window.moganOnClosePopup = cb;
  return () => {
    if (window.moganOnClosePopup === cb) window.moganOnClosePopup = undefined;
  };
}

/** Run a menu item's command by its registered id (deferred via C++ queue). */
export function invokeMenu(id: number): void {
  ccall('mogan_menu_invoke', null, ['number'], [id]);
}

/** Tell C++ to close the active context menu (outside click / Esc). */
export function closePopup(): void {
  ccall('mogan_menu_close_popup', null, [], []);
}

/**
 * Request a lazily-expanded submenu's children. C++ forces the promise and
 * pushes the children back via the moganOnSubmenu listeners.
 */
export function requestSubmenu(id: number): void {
  ccall('mogan_menu_expand', null, ['number'], [id]);
}

// Multiple submenu components subscribe at once (one per open submenu row), so
// this is a multicast set rather than the single-slot registrar used by the
// whole-tree/footer/popup callbacks. The single window.moganOnSubmenu hook
// fans out to every registered listener; each listener filters by submenu id.
const submenuListeners = new Set<SubmenuListener>();
window.moganOnSubmenu = (id, children) => {
  submenuListeners.forEach((cb) => cb(id, children));
};

/** Subscribe to lazily-expanded submenu children pushed from C++. */
export function subscribeSubmenu(cb: SubmenuListener): () => void {
  submenuListeners.add(cb);
  return () => {
    submenuListeners.delete(cb);
  };
}

/**
 * Report the menu and footer pixel heights so the C++ main loop can position
 * the ImGui document canvas between them. Called on mount and on resize.
 */
export function setChromeMetrics(menuHeight: number, footerHeight: number): void {
  ccall(
    'mogan_set_chrome_metrics',
    null,
    ['number', 'number'],
    [menuHeight, footerHeight],
  );
}

/** Internal: best-effort ccall into the Emscripten module. */
function ccall(
  name: string,
  returnType: string | null,
  argTypes: string[],
  args: unknown[],
): void {
  try {
    window.Module?.ccall?.(name, returnType, argTypes, args);
  } catch {
    // Module not ready yet, or function not exported — ignore (menu/footer
    // events may arrive before the WASM runtime finishes booting).
  }
}
