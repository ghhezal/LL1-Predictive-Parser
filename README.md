# LL(1) Predictive Parser

A C implementation of an LL(1) predictive parser built from scratch: it computes FIRST and FOLLOW sets, constructs a predictive parsing table, checks the grammar for LL(1) conflicts, and parses an input string against the table while building a syntax tree.

## Features

- **FIRST/FOLLOW set computation:** fixed-point algorithm over a context-free grammar with epsilon (`ep`) support.
- **Predictive parsing table construction:** builds the table from the computed FIRST/FOLLOW sets and detects FIRST/FIRST and FIRST/FOLLOW conflicts.
- **LL(1) conflict detection:** reports the exact state and input symbol where a grammar fails to be LL(1), instead of just rejecting it outright.
- **Table-driven parsing:** parses an input string using a stack-based predictive parser, printing the stack/input/action trace at every step (the classic compilers-course table format).
- **Syntax tree construction:** builds and prints a parse tree for any accepted string.
- **Grammar loaded from file:** no recompilation needed to test a different grammar.

## How It Works

Nonterminals are represented as integer **states** (`0`, `1`, `2`, ...) rather than letters, which keeps the parser generic and avoids hardcoding any specific grammar. Each production is written as `state->body`, where `body` is a space-separated sequence of:

- another state number (a nonterminal), or
- a terminal symbol exactly as declared in the terminals line, or
- `ep` for epsilon (the empty string)

## Grammar File Format (`grammar.txt`)

```
<start state>
<terminal1> <terminal2> ... <terminalN>
<state>-><body>
<state>-><body>
...
```

- **Line 1**: the start state number (informational; parsing always begins at state `0`).
- **Line 2**: all terminal symbols, space-separated.
- **Line 3 onward**: one production per line, `state->body`. Multiple productions for the same state are written as separate lines.

### Example: classic expression grammar

```
E  → T E'
E' → + T E' | ε
T  → F T'
T' → * F T' | ε
F  → ( E ) | id
```

mapped to states `E=0, T=1, E'=2, T'=4, F=3`, this becomes the included `grammar.txt`:

```
0
id + * ( )
0->1 2
2->+ 1 2
2->ep
1->3 4
4->* 3 4
4->ep
3->( 0 )
3->id
```

## Getting Started

### Prerequisites

A C compiler (GCC or Clang).

### Build

```bash
gcc LL1-Predictive-Parser.c -o parser
```

### Run

```bash
./parser
```

The program reads `grammar.txt` from the current directory automatically. A sample grammar (the expression grammar above) is included in this repo.

It will print the FIRST/FOLLOW sets, the predictive parsing table, and a conflict check. If the grammar is LL(1), it then prompts you for a string to parse:

```
Enter the string to be analyzed: id+id*id
```

## Example Output

**FIRST / FOLLOW sets:**

```
State      | First                          | Follow
--------------------------------------------------------------------------------
 0          | (, id                          | ), $
 1          | id, (                          | ), $, +
 2          | +, ep                          | ), $
 3          | (, id                          | ), +, $, *
 4          | *, ep                          | ), +, $
```

**Predictive parsing table:**

```
State | id        | +         | *         | (         | )         | $
------------------------------------------------------------------------------------
 0    | 0->1 2    |           |           | 0->1 2    |           |
 1    | 1->3 4    |           |           | 1->3 4    |           |
 2    |           | 2->+ 1 2  |           |           | 2->ep     | 2->ep
 3    | 3->id     |           |           | 3->( 0 )  |           |
 4    |           | 4->ep     | 4->* 3 4  |           | 4->ep     | 4->ep
This grammar is LL(1).
```

**Parsing `id+id*id`** prints the full stack/input/action trace, then the resulting syntax tree.

## Known Limitations

- Multi-character terminals are matched by longest-match against the declared terminal list, so terminal names must be declared in line 2 exactly as they appear in the input string.
- The input string for parsing (`Analye`) is capped at 199 characters (`inputBuffer[200]`).
- Production bodies are capped at 99 characters (`Transition.body[100]`).
- Each tree node supports up to 10 children (`TreeNode.children[10]`).

## Project Structure

```
.
├── LL1-Predictive-Parser.c  # Main source file
├── grammar.txt              # Sample grammar (expression grammar)
├── .gitignore
└── README.md
```

## Author

**Amine Ghezal**
