import { Fragment, useState } from 'react';
import type { MenuNode } from './types';
import { invokeMenu } from './bridge';

/**
 * Shared renderer for a list of menu nodes — used by both the top-level
 * dropdowns (MenuDropdown) and the context menu. Mirrors the kinds handled by
 * render_node() in src/Plugins/ImGui/im_menu.cpp.
 *
 * `onInvoke` runs after a leaf button is clicked (lets the parent close its
 * dropdown / popup).
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

  return (
    <>
      {normalized.map((node, i) => (
        <MenuRow key={i} node={node} onInvoke={onInvoke} depth={depth} />
      ))}
    </>
  );
}

function MenuRow({
  node,
  onInvoke,
  depth,
}: {
  node: MenuNode;
  onInvoke?: () => void;
  depth: number;
}) {
  const [open, setOpen] = useState(false);

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
              <MenuRow key={i} node={child} onInvoke={onInvoke} depth={depth} />
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
          onMouseEnter={() => setOpen(true)}
          onMouseLeave={() => setOpen(false)}
        >
          <span className="mogan-menu-check" />
          <span className="mogan-menu-label">{node.label}</span>
          <span className="mogan-menu-arrow">▶</span>
          {open && (
            <ul className="mogan-menu-dropdown nested" role="menu">
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
