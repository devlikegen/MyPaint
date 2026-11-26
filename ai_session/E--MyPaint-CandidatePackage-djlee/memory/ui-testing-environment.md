---
name: ui-testing-environment
description: "How MyPaint UI can (and cannot) be driven for testing on this machine — Win32 messages work, cursor moves are refused"
metadata:
  node_type: memory
  type: project
  originSessionId: c644a546-ca80-43d4-a78f-6869f2b1e518
  modified: 2026-11-26T06:26:46.539Z
---

MyPaint UI tests on this machine are done from PowerShell with Add-Type C# (SendMessage/PostMessage), not computer-use (request_access rejected "MyPaint" as not installed).

- Buttons: PostMessage BM_CLICK (post, not send, when it opens a modal dialog). Canvas: mouse messages to control id 1014.
- File dialogs: set the name with WM_SETTEXT on the Edit id 1001 (save) or the Edit under ComboBoxEx32 id 1148 (open), then BM_CLICK id 1.
- SetCursorPos is refused here (returns FALSE, error 203), so hover effects such as tooltips cannot be simulated; check them with TTM_GETTOOLCOUNT or ask the user to hover.
- Keyboard shortcuts via keybd_event/SendInput only work while MyPaint is the foreground window; guard with GetForegroundWindow, which usually holds right after launch.
- The user may be using the machine during tests (cursor moving, windows minimized from outside), so keep topmost/capture popups brief.

**Why:** found while testing the file buttons and Ctrl+O on 2026-11-26; earlier, keys sent without a guard went to another app's window.
**How to apply:** reuse this approach for later UI checks, and report hover-only behavior as not verified instead of retrying cursor moves.
