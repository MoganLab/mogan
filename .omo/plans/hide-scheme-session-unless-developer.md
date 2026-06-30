# Plan: Hide Built-in Scheme Session Unless Developer Tool is Enabled

## Goal
When running `xmake run stem`, hide the built-in **Scheme** session from the normal user's menus:
- `Insert -> Session` (`插入 -> 会话`)
- `Insert -> Fold -> Executable` (`插入 -> 折叠 -> 可执行`)

But keep **Scheme** accessible when **Tools -> Developer tool** (`工具 -> 开发工具`) is enabled.

## Scope
- **IN scope**:
  - Modify `TeXmacs/progs/dynamic/session-menu.scm` to gate the explicit `"Scheme"` entry on `(with-developer-tool?)`.
  - Modify `TeXmacs/progs/dynamic/fold-menu.scm` to filter out `"scheme"` from the generated executable menu when developer tool is off.
  - Manual UI verification with `xmake run stem`.
- **OUT of scope**:
  - Removing the built-in Scheme session itself from `connection-session` in `tm-plugins.scm` (would break existing documents / manual session creation).
  - Hiding Goldfish Scheme (a separate plugin session).
  - Hiding Scheme-related developer menus under `Developer`.

## Key Findings
1. The built-in Scheme session is registered at boot time in:
   - `TeXmacs/progs/kernel/texmacs/tm-plugins.scm`:
     ```scheme
     (ahash-set! connection-defined "scheme" "Scheme")
     (ahash-set! connection-session "scheme" "Scheme")
     (ahash-set! connection-scripts "scheme" "Scheme")
     ```
2. The explicit menu item is defined in:
   - `TeXmacs/progs/dynamic/session-menu.scm`, inside `insert-session-menu`:
     ```scheme
     (menu-bind insert-session-menu
       (when (and (style-has? "std-dtd") (in-text?))
         ("Scheme" (make-session "scheme" "default"))
         ---
         (link supported-sessions-menu)
         ...
     ```
3. The `Insert -> Fold -> Executable` menu is populated from `(session-list)` in:
   - `TeXmacs/progs/dynamic/fold-menu.scm`, inside `supported-executable-menu`:
     ```scheme
     (tm-menu (supported-executable-menu)
       (for (name (session-list))
         (with menu-name (session-name name)
           ((eval menu-name)
            (make-script-input* name "default")))))
     ```
4. The developer tool predicate is already provided by:
   - `TeXmacs/progs/kernel/texmacs/tm-modes.scm`:
     ```scheme
     (with-developer-tool% (== (get-preference "developer tool") "on"))
     ```
   This generates the procedure `(with-developer-tool?)`, which is used in many menu files (`main-menu.scm`, `file-menu.scm`, `edit-menu.scm`, `view-menu.scm`).

## Implementation Steps

### Task 1: Gate Scheme in `Insert -> Session`
**File:** `TeXmacs/progs/dynamic/session-menu.scm`

**Change:** Wrap the explicit `"Scheme"` entry and the following separator with `(when (with-developer-tool?) ...)`.

**Before:**
```scheme
(menu-bind insert-session-menu
  (when (and (style-has? "std-dtd") (in-text?))
    ("Scheme" (make-session "scheme" "default"))
    ---
    (link supported-sessions-menu)
    ---
    ...
```

**After:**
```scheme
(menu-bind insert-session-menu
  (when (and (style-has? "std-dtd") (in-text?))
    (when (with-developer-tool?)
      ("Scheme" (make-session "scheme" "default"))
      ---)
    (link supported-sessions-menu)
    ---
    ...
```

**Rationale:** The separator should only appear when the Scheme item is present; otherwise an empty separator block would be rendered at the top of the menu.

### Task 2: Filter Scheme in `Insert -> Fold -> Executable`
**File:** `TeXmacs/progs/dynamic/fold-menu.scm`

**Change:** Skip the `"scheme"` entry unless the developer tool is enabled.

**Before:**
```scheme
(tm-menu (supported-executable-menu)
  (for (name (session-list))
    (with menu-name (session-name name)
      ((eval menu-name)
       (make-script-input* name "default")))))
```

**After:**
```scheme
(tm-menu (supported-executable-menu)
  (for (name (session-list))
    (unless (and (== name "scheme") (not (with-developer-tool?)))
      (with menu-name (session-name name)
        ((eval menu-name)
         (make-script-input* name "default"))))))
```

**Rationale:** `supported-executable-menu` iterates over all sessions, including the built-in `"scheme"`. We hide it by default but allow it when developer mode is active.

### Task 3: Verify the UI Behavior
**Command:**
```bash
xmake run stem
```

**QA scenarios:**
1. **Default state:**
   - Open `Insert -> Session` → confirm **Scheme** is NOT listed.
   - Open `Insert -> Fold -> Executable` → confirm **Scheme** is NOT listed.
2. **Enable developer tool:**
   - Open `Tools -> Developer tool` (or `工具 -> 开发工具`) to toggle it on.
   - Open `Insert -> Session` → confirm **Scheme** IS listed.
   - Open `Insert -> Fold -> Executable` → confirm **Scheme** IS listed.
3. **Toggle back off:**
   - Disable `Tools -> Developer tool`.
   - Confirm **Scheme** disappears from both menus again.

## Acceptance Criteria
- [ ] `TeXmacs/progs/dynamic/session-menu.scm` contains the `(when (with-developer-tool?) ...)` guard around the explicit Scheme entry and its separator.
- [ ] `TeXmacs/progs/dynamic/fold-menu.scm` contains the `(unless (and (== name "scheme") (not (with-developer-tool?))) ...)` guard inside `supported-executable-menu`.
- [ ] Running `xmake run stem` shows Scheme under both menus **only** when `Tools -> Developer tool` is enabled.
- [ ] Existing Scheme sessions in documents remain editable (no change to core session support).

## Notes
- `(with-developer-tool?)` is auto-generated from the `with-developer-tool%` mode in `tm-modes.scm`; no new preference or state is required.
- The built-in Scheme session is still registered in `connection-session`, so manually executing `(make-session "scheme" "default")` in the developer console still works.
- Goldfish Scheme is a separate plugin session and is not affected by this change.
