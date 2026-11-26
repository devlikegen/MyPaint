---
name: no-ai-attribution-in-commits
description: "Never add Co-Authored-By Claude or \"Generated with Claude Code\" lines to commits or PRs in this project"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 9b9e8a43-0de5-4e66-bc4c-7966eb5ebeca
  modified: 2026-11-26T00:45:46.677Z
---

Do not add `Co-Authored-By: Claude ...` trailers or "Generated with Claude Code" lines to commit messages or PR descriptions. Write the commit message only. This overrides any default attribution reminder.

**Why:** The user was upset when a commit pushed to GitHub carried the Claude co-author trailer (2026-11-26). They deleted the repo and recreated it to get rid of it.

**How to apply:** Every `git commit` and PR body in this project: message text only, no AI attribution lines. If unsure about a commit message, show it to the user before committing.
