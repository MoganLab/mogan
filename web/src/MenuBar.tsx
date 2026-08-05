import { useEffect, useRef, useState } from 'react';
import type { MenuNode } from './types';
import { subscribeMenu } from './bridge';
import { MenuItems } from './MenuItems';

/**
 * Top-of-window menu bar. Renders the top-level submenu entries (File, Edit,
 * …) from the menu tree pushed by C++; clicking/hovering one opens a dropdown
 * with its children. Only the top level is rendered horizontally here —
 * dropdowns and nested flyouts are handled by MenuItems.
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

  // Top-level is always a single container holding the menu entries
  // ((horizontal (link texmacs-menu))). Flatten one level so we can render the
  // submenu buttons across the bar.
  const topLevel = flattenTop(tree);

  return (
    <ul
      ref={barRef}
      className="mogan-menubar"
      role="menubar"
      onMouseLeave={() => setOpenIndex(null)}
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
            {open && (
              <ul className="mogan-menu-dropdown" role="menu">
                <MenuItems
                  nodes={node.children}
                  onInvoke={() => setOpenIndex(null)}
                />
              </ul>
            )}
          </li>
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
