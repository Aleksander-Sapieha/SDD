# Contributing guidelines

## General rules

1. Every contriburtor must use indentation, exactly 4 spaces.
2. Never use tabs for indentation.
3. Never copy copyrighted code from anywhere.
4. Never submit untested/broken code.
5. Usage of tools such as ChatGPT or Github Copilot is allowed, but must be disclosed.
6. Never leave unused code. Either comment it out, or remove it.

## Naming

1. Class names must have every first letter capitalised. Example:
```py
class LexerError(Exception):
    pass

```
2. Variables and function names must be lowercase, and must use "_" between every word. Example:
```py
def load_file(source):
    pass

```
## Language specific rules

### Python

1. Wildcard imports should never be used. 

### C++

1. Never use C-style labels.
2. Never use goto.
3. Always use "#pragma once" on top of a header file.
4. Only use .h files if they don't include any of C++ exclusive functionality. It they do, use .hpp instead.

### XML

1. XML 1.1 must always be used. Never use an older version.

### SDD

1. SDD 1.0 must always be used.
2. Only use SDD libraries from the [official github repository](https://github.com/Aleksander-Sapieha/SDD)
3. Follow the current SDD standard available [here](https://aleksander-sapieha.github.io/sdd-website/sites/standards.html)

### New rules can come at any moment. After that happens, all existing code must be fixed as soon as possible