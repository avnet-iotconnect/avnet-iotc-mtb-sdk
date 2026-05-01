# fork-aivision C Style (compact)

Scope: Files that we own and/or create.

## Precedence

1. This file for formatting/layout decisions.
2. `CONTRIBUTING.md` for general conventions.

If rules conflict, follow this file.

When reformatting, respect what you may identify to be a reasonable concious decision to violate a rule vs. a simple mistake, lazyness or AI slop. Readability over consistency. Examples:
- A repeat `if failed then go to error handler` can go on one line if the same thing repeats often enough to warrant a one-liner.
- An above example may except the required curly braces.
- A function declaration is too long but args on the right are the same as another visible one-line function declaration. No need to "see" them in this case. An opposite may be a valid exception as well.

In other words, any conscious exceptions are acceptable, but slop is not. Long are the days when auto-formatting caused commit churn. IDE AI assists now follow your style patterns. But don't push your patterns in code "just beause you like it" - hence this document.

## Required

- Indent with 4 spaces. No tabs in new/edited lines.
- Use K&R/1TBS braces for functions and control blocks:

```c
static void run_task(void *arg) {
    if (arg == NULL) {
        return;
    }
}
```

- Allowed Exception: Brace may be on a line by itself following the function definition, if a part of an official library that we provide or if preferred by the developer.

- Names:
  - functions/variables/files: `snake_case`
  - type names (`typedef struct`, enums, fn-pointer typedefs): `CamelCase`
  - macros/constants: `UPPER_SNAKE_CASE`
- Exception Allowed: For type names `snake_case` or `snake_case_t` is allowed if vendor code prefers it, but the exception should be avoided when code (or code pattern) is intended to be shared across different vendor platforms (an SDK or common libs)  
- Prefer early returns for guard/error paths; use one `goto cleanup` block when resource unwinding is needed.
- goto is acceptable for error handling common cleanup.
- Keep one statement per line.

## Line breaking

Target readable width: up to about 160 columns. Over 160 is allowed for "I don't care to see this code often" for example ona long cookie-cutter variable assignment or declaration.

- Keep short calls/signatures/conditions on one line.
- If a line gets long, use this continuation form (not stair-step) when more than a handful of args need to be broken up (case when many args):
- if/else and similar generally should not omit curly braces. Always, if followed on the next line. An example rare exception may be a repeat "if (fail) goto handler;"

```c
result = some_function(
    arg1,
    arg2,
    arg3
);
```

```c
if (
    cond_a &&
    cond_b &&
    cond_c
) {
    do_work();
}
```

- Grouping of related args/statment on the same line is allowed when logically related.

- Preferred alternative especially for printf format and when only a few args or grouping improves readability. Not allowed for function parameters:

```c
printf("Result: %d, error: more long text ... %d\n",
   result, error_code
);

```
- Do not collapse already-readable multiline calls into a single long line.
Column width is a readability guideline, not a hard limit.
- Aim for condensed code (less LOC). Avoid empty lines, but add them to accent logical grouping or decoupling.
- Aim for <= 160 columns for most code.
- It is acceptable to exceed 160 when breaking the line would reduce readability or add noisy wrapping.
- The general wrapping rule is "do I really need to see what's far to the right" or will I benefit more from seeing more important LOCs. 
- Typical exception: long `printf(...)`/logging calls where arguments are straightforward and already easy to scan especially when many other printfs in the file.
- Wrap lines when structure must be visually parsed (long conditions, nested calls, many non-trivial arguments, or mixed expressions).
- When wrapping, use the node-style continuation form defined below (not stair-step).


## Headers

- New C/H files we own use SPDX header:

```c
/* SPDX-License-Identifier: MIT
 * Copyright (C) 2026 Avnet
 * Authors: Author Name <author.name@avnet.com> et al.
 */
```

- Agents should pull author info from git global cofig.

- When editing vendor files, keep vendor header and append SPDX line if needed.

## Comments

- Comments, and even short blocks use `//` while `/*` blocks can be used for
multi-line (usually over 3) so that they can be edted easily. Examples: Code snippets, large descriptions with formatting.
- AI should not use numbered steps when describing flow.
- Comment the why, not the obvious what.
- When API call (usually in headers) is complicated provide call examples.
- Keep comments short and local to non-obvious logic.
- Never use block comments with standard "parameters" and "returns" unless a library code and probably not even then.

## Other
- No vertical alignment padding (types, names, assignments, args). Use single spaces only.
- Exception to the above: A list of #define constants can be aligned if repeats enough.

## Legacy/reflow policy

- Avoid drive-by reformatting.
- Reflow only touched blocks.
- Never perform style-only mass rewrites in functional patches.

