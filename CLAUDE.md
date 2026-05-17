# Project notes for Claude

## Formatting

In addition to the global C/C++ rules in `~/.claude/CLAUDE.md`:

- **Column alignment uses raw tabs first, spaces only as a tie-breaker** when
  the next tab stop would overshoot the target column. This applies to
  alignment of `=` in declaration lists, `{` after method signatures,
  trailing comments after fields, etc. Never use a run of spaces to align
  columns when a tab (or tabs) can land on or near the target. Tab width is
  8 (`ts=8`).
