/**
 * Shared data shapes between the C++ ImGui backend and the React shell.
 *
 * The C++ side serializes its im_menu_rep widget tree (the same tree the native
 * ImGui build renders directly) to JSON and pushes it here via the bridge in
 * bridge.ts. Every leaf button carries a stable integer id; clicking calls back
 * into C++ via Module.ccall('mogan_menu_invoke', id).
 */

/** One node in a menu tree (main menu, submenu, or context menu). */
export type MenuNode =
  | { kind: 'container'; children: MenuNode[] }
  | { kind: 'submenu'; label: string; children: MenuNode[] }
  | {
      kind: 'button';
      id: number;
      label: string;
      /** Shortcut string for display only (e.g. "Ctrl+S"); omitted when empty. */
      shortcut?: string;
      checked: boolean;
      enabled: boolean;
    }
  | { kind: 'separator' }
  | { kind: 'group'; label: string }
  | { kind: 'text'; label: string };

/** Three-column status bar state pushed from the editor. */
export interface FooterState {
  left: string;
  middle: string;
  right: string;
  /** Reserved for a future minibuffer-style input mode (not yet rendered). */
  interactive: boolean;
}
