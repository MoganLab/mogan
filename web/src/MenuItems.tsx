import {
  Fragment,
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';
import { createPortal } from 'react-dom';
import type { MenuNode } from './types';
import { invokeMenu, requestSubmenu, subscribeSubmenu } from './bridge';

/**
 * Resolve a submenu's children, re-requesting them from C++ EVERY time the
 * submenu opens — mirroring Qt's QTMLazyMenu, whose aboutToShow re-forces the
 * promise so the menu always reflects current state (no stale snapshot).
 *
 * Each submenu component owns its children state. `ensure()` fires a fresh
 * requestSubmenu(id); when C++ pushes the branch back, the matching component
 * (by id) updates its own state. Previously-shown children keep rendering
 * until the fresh ones arrive, so re-opening doesn't flash an empty menu.
 */
export function useSubmenuChildren(node: MenuNode & { kind: 'submenu' }): {
  children: MenuNode[] | undefined;
  ensure: () => void;
} {
  // Local copy of the latest children for THIS submenu. Seed from the node's
  // embedded children if present (eagerly-serialized case).
  const [children, setChildren] = useState<MenuNode[] | undefined>(
    node.children,
  );
  // Track in-flight request to avoid spamming C++ while one is pending.
  const inflight = useRef(false);

  // Subscribe once; only adopt children addressed to this submenu's id.
  useEffect(() => {
    if (node.id === undefined) return;
    return subscribeSubmenu((id, kids) => {
      if (id === node.id) {
        inflight.current = false;
        setChildren(kids);
      }
    });
  }, [node.id]);

  const ensure = () => {
    if (node.id === undefined || inflight.current) return;
    inflight.current = true;
    requestSubmenu(node.id);
  };
  return { children, ensure };
}

/**
 * Shared renderer for a list of menu nodes — used by both the top-level
 * dropdowns (MenuDropdown) and the context menu. Mirrors the kinds handled by
 * render_node() in src/Plugins/ImGui/im_menu.cpp.
 *
 * `onInvoke` runs only after an ENABLED leaf button actually fires its
 * command (it lets the parent close the whole menu). Inert clicks do not
 * trigger it, so the menu stays open when the user clicks a disabled item, a
 * group/section header, a separator, or a submenu row that only expands.
 *
 * Submenu (flyout) open state is managed PER LIST (this component), not per
 * row: only one flyout per level is open at a time, and hovering a sibling
 * closes the previous one. An open flyout NEVER collapses just because the
 * pointer left it — it closes only when a sibling flyout opens or the whole
 * menu is dismissed. Clicking a submenu row runs no command, so it never
 * closes the surrounding menu.
 *
 * Every dropdown layer stops its own mousedown from bubbling: the dismiss
 * listeners on MenuBar / ContextMenu listen at window capture time, and a
 * click inside a nested flyout would otherwise look "outside" the parent
 * dropdown it isn't DOM-nested under. Marking it here keeps such clicks from
 * being treated as an outside dismiss (and swallowed).
 */
interface MenuItemsProps {
  nodes: MenuNode[];
  /** Called after a leaf command runs. */
  onInvoke?: () => void;
  /** Nesting depth, for styling nested flyouts. */
  depth?: number;
}

export function MenuItems({ nodes, onInvoke, depth = 0 }: MenuItemsProps) {
  // Collapse leading / trailing / consecutive separators, matching the
  // normalization in im_menu.cpp render_node k_container.
  const normalized = normalizeSeparators(nodes);

  // Index (into `normalized`) of the currently-open submenu at THIS level.
  // One open flyout per level. Hovering a sibling submenu switches to it;
  // hovering ANY other (non-submenu) row closes the open flyout. An open
  // flyout never collapses just because the pointer left the menu entirely.
  const [openIdx, setOpenIdx] = useState<number | null>(null);
  const closeFlyout = () => setOpenIdx(null);

  return (
    <>
      {normalized.map((node, i) => (
        <MenuRow
          key={i}
          node={node}
          onInvoke={onInvoke}
          depth={depth}
          open={openIdx === i}
          onHoverOpen={() => setOpenIdx(i)}
          onHoverClose={() => {
            // A flyout never collapses just because the pointer left it — it
            // stays open until the user hovers a sibling (handled by
            // onHoverOpen / onHoverSibling) or dismisses the whole menu.
          }}
          onHoverSibling={closeFlyout}
          onClickRow={() => {
            // Clicking a submenu row just opens its flyout. No command runs,
            // so onInvoke is not called and the surrounding menu stays open.
            setOpenIdx(i);
          }}
        />
      ))}
    </>
  );
}

function MenuRow({
  node,
  onInvoke,
  depth,
  open,
  onHoverOpen,
  onHoverClose,
  onHoverSibling,
  onClickRow,
}: {
  node: MenuNode;
  onInvoke?: () => void;
  depth: number;
  /** Whether this row's flyout is open (only meaningful for submenu rows). */
  open: boolean;
  onHoverOpen: () => void;
  onHoverClose: () => void;
  /** Fired when a NON-submenu sibling is hovered (closes the open flyout). */
  onHoverSibling: () => void;
  onClickRow: () => void;
}) {
  switch (node.kind) {
    case 'container':
      return <MenuItems nodes={node.children} onInvoke={onInvoke} depth={depth} />;

    case 'tile':
      // Grid of equally-sized cells (e.g. math symbol palettes). Column width
      // fits the widest label (max-content) rather than stretching to the
      // menu width, and the label is centered in its cell.
      return (
        <li
          className="mogan-menu-tile"
          role="presentation"
          style={{ gridTemplateColumns: `repeat(${Math.max(1, node.cols)}, max-content)` }}
        >
          {node.children.map((child, i) =>
            child.kind === 'button' ? (
              <button
                key={i}
                type="button"
                className={'mogan-menu-tile-cell' + (child.enabled ? '' : ' disabled')}
                disabled={!child.enabled}
                title={child.label}
                onClick={() => {
                  // Close first, run the command deferred — see the button
                  // case below for why (a synchronous ccall command can block
                  // the main thread and delay the close repaint).
                  onInvoke?.();
                  setTimeout(() => invokeMenu(child.id), 0);
                }}
              >
                {child.label}
              </button>
            ) : (
              <span key={i} className="mogan-menu-tile-cell">
                <MenuRow
                  node={child}
                  onInvoke={onInvoke}
                  depth={depth}
                  open={false}
                  onHoverOpen={() => {}}
                  onHoverClose={() => {}}
                  onHoverSibling={() => {}}
                  onClickRow={() => {}}
                />
              </span>
            ),
          )}
        </li>
      );

    case 'separator':
      return (
        <li
          className="mogan-menu-separator"
          role="separator"
          onMouseEnter={onHoverSibling}
        />
      );

    case 'group':
      return (
        <li className="mogan-menu-group" onMouseEnter={onHoverSibling}>
          {node.label}
        </li>
      );

    case 'text':
      return (
        <li className="mogan-menu-text" onMouseEnter={onHoverSibling}>
          {node.label}
        </li>
      );

    case 'button':
      return (
        <li
          className={'mogan-menu-item' + (node.enabled ? '' : ' disabled')}
          role="menuitem"
          onMouseEnter={onHoverSibling}
          onClick={() => {
            if (!node.enabled) return;
            // Close the menu FIRST (synchronous setState), then defer the
            // command. invokeMenu -> ccall -> mogan_menu_invoke runs the C++
            // command synchronously, and a heavy command (menu rebuild /
            // repaint) would block the main thread and delay the close
            // repaint, making the menu feel stuck. Running it after the close
            // state has been queued lets the menu vanish immediately.
            onInvoke?.();
            setTimeout(() => invokeMenu(node.id), 0);
          }}
        >
          <span className="mogan-menu-label">{node.label}</span>
          <span className="mogan-menu-shortcut">{node.shortcut ?? ''}</span>
          {/* Check mark shown at the END of the row (after the shortcut), per
              preference — no left check column, so rows sit flush left. */}
          <span className="mogan-menu-check-end">{node.checked ? '✓' : ''}</span>
        </li>
      );

    case 'submenu':
      return (
        <SubmenuRow
          node={node}
          open={open}
          onHoverOpen={onHoverOpen}
          onHoverClose={onHoverClose}
          onClickRow={onClickRow}
          onInvoke={onInvoke}
          depth={depth}
        />
      );

    default:
      return <Fragment />;
  }
}

/**
 * Ref callback + className helper for a dropdown <ul>: after mount / whenever
 * `deps` change, mark the element .can-scroll when its content actually
 * overflows its (capped) visible height. The CSS appends the small end spacer
 * only for .can-scroll menus, so short menus get no stray blank row.
 */
export function useCanScrollClass<T extends HTMLElement>(deps: unknown[]) {
  const ref = useRef<T | null>(null);
  const [cls, setCls] = useState('');
  useLayoutEffect(() => {
    const el = ref.current;
    if (!el) return;
    setCls(el.scrollHeight > el.clientHeight + 1 ? ' can-scroll' : '');
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, deps);
  return { ref, cls };
}

/**
 * A submenu row + its flyout. The flyout flips to grow upward (bottom-aligned
 * to the row) when the row sits low enough that a downward-opening flyout
 * would overflow the viewport — decided from the row's position each time the
 * flyout opens, so it doesn't need a render during the same pass.
 */
function SubmenuRow({
  node,
  open,
  onHoverOpen,
  onHoverClose,
  onClickRow,
  onInvoke,
  depth,
}: {
  node: MenuNode & { kind: 'submenu' };
  open: boolean;
  onHoverOpen: () => void;
  onHoverClose: () => void;
  onClickRow: () => void;
  onInvoke?: () => void;
  depth: number;
}) {
  const rowRef = useRef<HTMLLIElement>(null);
  const flyout = useCanScrollClass<HTMLUListElement>([open]);
  // Lazily-expanded children (see useSubmenuChildren): undefined until the
  // submenu is first opened and C++ pushes the branch.
  const { children, ensure: ensureChildren } = useSubmenuChildren(node);
  // Viewport coordinates for the fixed-position flyout. `top` starts aligned
  // with the row; after the flyout renders we measure its real height and, if
  // it would overflow the viewport bottom, shift it up JUST enough to fit —
  // keeping it as close to the parent row as possible.
  const [pos, setPos] = useState<{ left: number; top: number }>({
    left: 0,
    top: 0,
  });

  const measure = () => {
    const el = rowRef.current;
    if (!el) return;
    const rect = el.getBoundingClientRect();
    // Clamp left so the flyout never starts past the right viewport edge.
    const left = Math.min(
      Math.round(rect.right + 2),
      Math.max(8, window.innerWidth - 220),
    );
    // Clamp top into the visible viewport even when the row itself is
    // scrolled outside it (a long, scrollable parent menu): the flyout is
    // fixed-position and must stay on screen regardless of the row's rect.
    let top = Math.round(rect.top - 4);
    const maxTop = Math.max(8, window.innerHeight - 48);
    if (top > maxTop) top = maxTop;
    if (top < 8) top = 8;
    setPos({ left, top });
  };

  // Keep the flyout as close to the row as possible while leaving a usable
  // amount of it on screen: if aligning to the row would push the flyout so
  // low that fewer than ~120px (a couple of rows) remain visible, shift it up
  // so that much fits. The bottom is then kept inside the viewport by the
  // inline maxHeight, and any taller content scrolls.
  useLayoutEffect(() => {
    if (!open) return;
    setPos((p) => {
      const minVisible = 120;
      const maxTop = Math.max(8, window.innerHeight - 8 - minVisible);
      const top = Math.min(p.top, maxTop);
      const left = Math.min(p.left, Math.max(8, window.innerWidth - 220));
      return top === p.top && left === p.left ? p : { left, top };
    });
  }, [open]);

  return (
    <li
      ref={rowRef}
      className={'mogan-menu-item submenu' + (open ? ' open' : '')}
      onMouseEnter={() => {
        measure();
        ensureChildren();
        onHoverOpen();
      }}
      onMouseLeave={onHoverClose}
      // Clicking a submenu row only opens/pins its flyout — it runs no
      // command, so onInvoke is NOT called and the menu stays open.
      onClick={() => {
        measure();
        ensureChildren();
        onClickRow();
      }}
    >
      <span className="mogan-menu-label">{node.label}</span>
      <span className="mogan-menu-arrow">▶</span>
      {open &&
        // Render the flyout into document.body via a portal so it fully
        // escapes the parent dropdown's overflow-y:auto scroll container.
        // Safari (unlike Chrome) clips position:fixed descendants against an
        // ancestor that has non-visible overflow, which is exactly why the
        // flyout was cut off at the parent menu's edge there. Portaling to
        // body removes that clip on both engines. Coordinates are already
        // viewport-based (position:fixed), so nothing else changes.
        createPortal(
          <ul
            ref={flyout.ref}
            className={'mogan-menu-dropdown mogan-menu-flyout' + flyout.cls}
            role="menu"
            style={{
              left: pos.left,
              top: pos.top,
              // Constrain by the space actually remaining below the flyout's
              // top (not a fixed 100vh), so the bottom never runs past the
              // viewport edge — content taller than that scrolls.
              maxHeight: Math.max(80, window.innerHeight - pos.top - 8),
            }}
            // See header: keep a click inside this flyout from looking like an
            // "outside" click to the window-level dismiss listener.
            onMouseDown={(e) => e.stopPropagation()}
          >
            {children === undefined ? (
              <li className="mogan-menu-text">…</li>
            ) : (
              <MenuItems nodes={children} onInvoke={onInvoke} depth={depth + 1} />
            )}
          </ul>,
          document.body,
        )}
    </li>
  );
}

function normalizeSeparators(nodes: MenuNode[]): MenuNode[] {
  const isSep = (n: MenuNode): boolean => n.kind === 'separator';
  const out: MenuNode[] = [];
  let prevSep = true; // drop leading separators
  for (const n of nodes) {
    if (isSep(n) && prevSep) continue;
    out.push(n);
    prevSep = isSep(n);
  }
  while (out.length > 0 && isSep(out[out.length - 1])) out.pop();
  return out;
}
