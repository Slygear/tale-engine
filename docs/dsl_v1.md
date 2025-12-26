# Tale Engine DSL v1 Specification

Version: 1.0 (initial)
Status: Draft (implementation target)

## Overview

Tale DSL is an indentation-based, human-readable language used to author narrative content.
DSL source files use the `.tale` extension.

This document defines the exact syntax and semantics for **DSL v1**.

## File Extension

- Source files: `*.tale`

## Whitespace and Indentation Rules

- Indentation is **spaces only**. Tabs are not allowed.
- The indentation unit is **2 spaces**.
- Block headers end with a colon (`:`) and must be followed by a newline.
- Blank lines are allowed anywhere.
- Comments start with `#` and continue to end-of-line.

Indentation errors must be reported with a precise source location (file, line, column).

## Identifiers

Identifiers are used for scene IDs, flag names, and item IDs.

Pattern:
- `[A-Za-z_][A-Za-z0-9_]*`

Examples:
- `intro`
- `castle_gate`
- `flag_has_sword`
- `iron_dagger`

## Literals

### String Literals

- Strings use double quotes: `"..."`.
- Supported escapes in v1:
  - `\"`
  - `\\`
  - `\n`
  - `\t`

### Integer Literals

- Base-10 integers, non-negative in v1.
- Example: `0`, `1`, `42`

### Boolean Literals

- `true`
- `false`

## Top-Level Structure

A file contains one or more scene declarations.

### Scene Declaration

