import { Fragment, useState } from 'react';
import type { MenuNode } from './types';
import { invokeMenu } from './bridge';

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
  // One open flyout per level; hovering a different submenu row switches to
  // it, and an open flyout never auto-collapses on pointer-leave — it only
  // closes when a sibling opens or the whole menu is dismissed.
  const [openIdx, setOpenIdx] = useState<number | null>(null);

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
            // stays open until the user hovers a sibling submenu (openIdx
            // moves) or dismisses the whole menu. Nothing to do here.
          }}
          onClickRow={() => {
            // Clicking a submenu row just opens its flyout (and keeps it
            // pinned via the no-auto-collapse rule above). No command runs, so
            // onInvoke is not called and the surrounding menu stays open.
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
  onClickRow,
}: {
  node: MenuNode;
  onInvoke?: () => void;
  depth: number;
  /** Whether this row's flyout is open (only meaningful for submenu rows). */
  open: boolean;
  onHoverOpen: () => void;
  onHoverClose: () => void;
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
                  invokeMenu(child.id);
                  onInvoke?.();
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
                  onClickRow={() => {}}
                />
              </span>
            ),
          )}
        </li>
      );

    case 'separator':
      return <li className="mogan-menu-separator" role="separator" />;

    case 'group':
      return <li className="mogan-menu-group">{node.label}</li>;

    case 'text':
      return <li className="mogan-menu-text">{node.label}</li>;

    case 'button':
      return (
        <li
          className={'mogan-menu-item' + (node.enabled ? '' : ' disabled')}
          role="menuitem"
          onClick={() => {
            if (!node.enabled) return;
            invokeMenu(node.id);
            onInvoke?.();
          }}
        >
          <span className="mogan-menu-check">{node.checked ? '✓' : ''}</span>
          <span className="mogan-menu-label">{node.label}</span>
          <span className="mogan-menu-shortcut">{node.shortcut ?? ''}</span>
        </li>
      );

    case 'submenu':
      return (
        <li
          className={'mogan-menu-item submenu' + (open ? ' open' : '')}
          onMouseEnter={onHoverOpen}
          onMouseLeave={onHoverClose}
          // Clicking a submenu row only opens/pins its flyout — it runs no
          // command, so onInvoke is NOT called and the menu stays open.
          onClick={onClickRow}
        >
          <span className="mogan-menu-check" />
          <span className="mogan-menu-label">{node.label}</span>
          <span className="mogan-menu-arrow">▶</span>
          {open && (
            <ul
              className="mogan-menu-dropdown nested"
              role="menu"
              // See header: keep a click inside this flyout from looking like an
              // "outside" click to the window-level dismiss listener (this flyout
              // isn't DOM-nested under its parent dropdown).
              onMouseDown={(e) => e.stopPropagation()}
            >
              <MenuItems nodes={node.children} onInvoke={onInvoke} depth={depth + 1} />
            </ul>
          )}
        </li>
      );

    default:
      return <Fragment />;
  }
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
