import { useEffect, useRef, useState } from 'react';
import type { KeyboardEvent } from 'react';
import type { DialogState } from './types';
import { cancelDialog, subscribeDialog, submitDialog } from './bridge';

/**
 * Interactive dialog modal (WASM). C++ pushes a DialogState whenever an
 * `interactive` command runs (e.g. collab new-document / configure-server) —
 * the WASM build bypasses the stubbed inputs_list_widget/dialogue_start path
 * and drives this modal instead. Renders one labeled input per prompt; Enter
 * submits, Esc / overlay-click cancels. Submitting calls back into C++, which
 * runs the pending scheme function with the values.
 */
export function Dialog() {
  const [state, setState] = useState<DialogState | null>(null);
  const inputsRef = useRef<Array<HTMLInputElement | null>>([]);

  useEffect(() => subscribeDialog(setState), []);

  // Reset input refs array length when a new dialog arrives.
  useEffect(() => {
    inputsRef.current = inputsRef.current.slice(0, state?.prompts.length ?? 0);
  }, [state]);

  if (!state) return null;

  const close = () => setState(null);

  const submit = () => {
    const values = state.prompts.map(
      (_, i) => inputsRef.current[i]?.value ?? '',
    );
    close();
    submitDialog(values);
  };
  const cancel = () => {
    close();
    cancelDialog();
  };

  const onKey = (e: KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      submit();
    } else if (e.key === 'Escape') {
      e.preventDefault();
      cancel();
    }
  };

  return (
    <div
      className="mogan-dialog-overlay"
      onMouseDown={(e) => {
        if (e.target === e.currentTarget) cancel();
      }}
    >
      <div
        className="mogan-dialog"
        role="dialog"
        aria-modal="true"
        onKeyDown={onKey}
      >
        <div className="mogan-dialog-title">{state.title}</div>
        {state.prompts.map((prompt, i) => (
          <label key={i} className="mogan-dialog-field">
            <span className="mogan-dialog-label">{prompt}</span>
            <input
              ref={(el) => {
                inputsRef.current[i] = el;
              }}
              type={state.types[i] === 'password' ? 'password' : 'text'}
              defaultValue={state.defaults[i] ?? ''}
              autoFocus={i === 0}
              className="mogan-dialog-input"
            />
          </label>
        ))}
        <div className="mogan-dialog-actions">
          <button type="button" onClick={cancel}>
            Cancel
          </button>
          <button
            type="button"
            className="mogan-dialog-primary"
            onClick={submit}
          >
            OK
          </button>
        </div>
      </div>
    </div>
  );
}
