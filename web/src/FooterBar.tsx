import { useEffect, useRef, useState } from 'react';
import type { FooterState } from './types';
import { subscribeFooter } from './bridge';

const EMPTY: FooterState = { left: '', middle: '', right: '', interactive: false };

/**
 * Bottom-of-window status bar. Three-column layout (left / centered middle /
 * right), matching the ImGui ##mogan_statusbar block it replaces. Content is
 * pushed from the editor via SLOT_LEFT/MIDDLE/RIGHT_FOOTER.
 *
 * Reports its pixel height to C++ on mount/resize via onHeight.
 */
export function FooterBar({ onHeight }: { onHeight: (h: number) => void }) {
  const [state, setState] = useState<FooterState>(EMPTY);
  const ref = useRef<HTMLElement>(null);

  useEffect(() => subscribeFooter(setState), []);

  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    const report = () => onHeight(el.getBoundingClientRect().height);
    report();
    const ro = new ResizeObserver(report);
    ro.observe(el);
    return () => ro.disconnect();
  }, [onHeight]);

  return (
    <footer ref={ref} className="mogan-footer">
      <span className="mogan-footer-left">{state.left}</span>
      <span className="mogan-footer-middle">{state.middle}</span>
      <span className="mogan-footer-right">{state.right}</span>
    </footer>
  );
}
