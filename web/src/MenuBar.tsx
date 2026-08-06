import { useEffect, useRef, useState } from 'react';
import type { MenuNode } from './types';
import { subscribeMenu } from './bridge';
import { MenuItems, useCanScrollClass, useSubmenuChildren } from './MenuItems';

/**
 * Top-of-window menu bar. Renders the top-level submenu entries (File, Edit,
 * …) from the menu tree pushed by C++; clicking one opens its dropdown.
 * Only the top level is rendered horizontally here — dropdowns and nested
 * flyouts are handled by MenuItems.
 *
 * Open semantics: a dropdown stays open once opened — it does NOT close when
 * the pointer leaves the bar. It closes only on an explicit gesture: picking
 * an item, pressing Escape, clicking another top-level entry, or a mousedown
 * anywhere outside the bar. That outside mousedown is swallowed in the
 * capture phase so it never falls through to the editor canvas.
 *
 * Reports its pixel height to C++ via setChromeMetrics on mount/resize so the
 * document canvas is positioned below it.
 */
export function MenuBar({ onHeight }: { onHeight: (h: number) => void }) {
  const [tree, setTree] = useState<MenuNode[]>([]);
  const [openIndex, setOpenIndex] = useState<number | null>(null);
  const barRef = useRef<HTMLUListElement>(null);

  useEffect(() => subscribeMenu(setTree), []);

  useEffect(() => {
    const el = barRef.current;
    if (!el) return;
    const report = () => onHeight(el.getBoundingClientRect().height);
    report();
    const ro = new ResizeObserver(report);
    ro.observe(el);
    return () => ro.disconnect();
  }, [onHeight]);

  // Close only on explicit dismiss: Escape, or a mousedown anywhere outside
  // the menu UI. The listener is window-level and checks every .mogan-menubar /
  // .mogan-menu-dropdown element, so it covers the bar, its dropdown, and any
  // nested flyouts alike — clicks inside any of them never close. An outside
  // mousedown is captured (and swallowed) BEFORE it reaches the canvas's GLFW
  // listener, so the click that closes the menu never reaches the editor.
  // Deliberately NOT closed on mouse-leave: the menu stays open until one of
  // these explicit gestures.
  useEffect(() => {
    if (openIndex === null) return;
    const onPointerDown = (e: MouseEvent) => {
      const inside = (e.target as Element | null)?.closest?.(
        '.mogan-menubar, .mogan-menu-dropdown',
      );
      if (!inside) {
        e.stopPropagation(); // swallow: don't let the click fall through to the canvas
        setOpenIndex(null);
      }
    };
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') setOpenIndex(null);
    };
    window.addEventListener('mousedown', onPointerDown, true);
    window.addEventListener('keydown', onKey, true);
    return () => {
      window.removeEventListener('mousedown', onPointerDown, true);
      window.removeEventListener('keydown', onKey, true);
    };
  }, [openIndex]);

  // Top-level is always a single container holding the menu entries
  // ((horizontal (link texmacs-menu))). Flatten one level so we can render the
  // submenu buttons across the bar.
  const topLevel = flattenTop(tree);

  return (
    <ul
      ref={barRef}
      className="mogan-menubar"
      role="menubar"
    >
      {topLevel.map((node, i) => {
        if (node.kind !== 'submenu') return null;
        const open = openIndex === i;
        return (
          <li
            key={i}
            className={'mogan-menubar-item' + (open ? ' open' : '')}
            role="menuitem"
            onMouseEnter={() => setOpenIndex(openIndex === null ? null : i)}
            onClick={() => setOpenIndex(open ? null : i)}
          >
            <span className="mogan-menu-label">{node.label}</span>
            {open && <TopDropdown node={node} onInvoke={() => setOpenIndex(null)} />}          </li>
        );
      })}
    </ul>
  );
}

/**
 * The C++ side wraps the top-level menu in a container. Extract its children;
 * if there's no container wrapper, return the input.
 */
function flattenTop(tree: MenuNode[]): MenuNode[] {
  if (tree.length === 1 && tree[0].kind === 'container') return tree[0].children;
  return tree;
}

/**
 * One top-level dropdown. Split into its own component so the can-scroll
 * detection hook can run per open dropdown (hooks can't be called inside the
 * parent's .map). The spacer class only appears when the menu truly overflows.
 */
function TopDropdown({
  node,
  onInvoke,
}: {
  node: MenuNode & { kind: 'submenu' };
  onInvoke: () => void;
}) {
  const dd = useCanScrollClass<HTMLUListElement>([]);
  // Top-level menus are also lazy: request children when the dropdown opens.
  const { children, ensure } = useSubmenuChildren(node);
  useEffect(() => {
    ensure();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);
  return (
    <ul ref={dd.ref} className={'mogan-menu-dropdown' + dd.cls} role="menu">
      {children === undefined ? (
        <li className="mogan-menu-text">…</li>
      ) : (
        <MenuItems nodes={children} onInvoke={onInvoke} />
      )}
    </ul>
  );
}
