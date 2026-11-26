# PYSDD

**SDD (Source's Data Document)** is a human-readable data format designed as an alternative to traditional data formats such as JSON and XML.

PYSDD is the official Python implementation of SDD. It provides tools for parsing, loading, serializing, and saving SDD documents.

> **Current Version:** 1.0 
> **Earliest SDD version supported:** 1.0  

## Features

- Human-readable syntax
- Explicit data types
- Nested objects
- Arrays and nested arrays
- Objects inside arrays
- UTF-8 support
- File loading and saving
- Python dictionary and list conversion
- Exact decimal representation using Python's `Decimal`

## Installation

Clone the repository:

```bash
git clone https://github.com/Aleksander-Sapieha/SDD.git
cd SDD
```

Import the library in Python:

```python
import PYSDD
```

## Basic SDD Syntax

Every SDD document starts with a version declaration:

```text
SDD 1.0
```

### Primitive values

```text
SDD 1.0

name is string "Source"
age is integer 17
version is decimal 1.5
active is boolean true
description is null
```

### Objects

Objects use the `contains` keyword and terminate with `end`.

```text
SDD 1.0

user contains
    name is string "Source"
    age is integer 17
    active is boolean true
end
```

### Arrays

Arrays use the `array` keyword and terminate with `end`.

```text
SDD 1.0

games is array
    string "MyGame"
    string "MyOS"
    string "SDD"
end
```

### Nested structures

Objects and arrays can be nested.

```text
SDD 1.0

players is array
    contains
        name is string "Source"
        level is integer 25
        inventory is array
            string "Sword"
            string "Shield"
        end
    end
end
```

## Python API

PYSDD provides four main functions.

### `PYSDD.load(filepath)`

Reads and parses an SDD file.

```python
import PYSDD

data = PYSDD.load("data.sdd")
print(data["values"]["name"])
```

### `PYSDD.loads(source)`

Parses PYSDD source code directly from a Python string.

```python
import PYSDD

source = '''
SDD 1.0

name is string "Source"
'''

data = PYSDD.loads(source)
```
### You can also use PYSDD.loadb to dump into a file using pickle

### `PYSDD.dump(data, filepath)`

Serializes Python data and writes it to an SDD file.

```python
import PYSDD

data = {
    "name": "Source",
    "age": 17,
    "active": True
}

PYSDD.dump(data, "output.sdd")
```

### `PYSDD.dumps(data)`

Converts Python data into an SDD string.

```python
import PYSDD

data = {
    "name": "Source",
    "age": 17
}

source = PYSDD.dumps(data)
print(source)
```

### You can also use PYSDD.dumpb to dump into a file using pickle

## Python Data Types

| SDD type | Python type |
|---|---|
| `string` | `str` |
| `integer` | `int` |
| `decimal` | `decimal.Decimal` |
| `boolean` | `bool` |
| `null` | `None` |
| `array` | `list` |
| Object | `dict` |

## Error Handling

PYSDD defines several error classes:

- `SDDError`
- `SDDSyntaxError`
- `SDDTypeError`
- `SDDDuplicateKeyError`
- `SDDVersionError`
- `SDDFileError`

These can be imported from the `PYSDD.errors` module.

## Round-trip Example

PYSDD can serialize Python data and parse it again.

```python
import PYSDD

original = {
    "name": "Source",
    "age": 17,
    "games": ["MyGame", "MyOS"]
}

encoded = PYSDD.dumps(original)
decoded = PYSDD.loads(encoded)

assert decoded["values"] == original

print("Round-trip successful!")
```

## Project Structure

```text
SDD/
├── PYSDD/
│   ├── __init__.py
│   ├── lexer.py
│   ├── parser.py
│   ├── serializer.py
│   ├── loader.py
│   ├── main.py
│   └── errors.py
├── main.py
├── README.md
└── LICENSE
```

## License

See the LICENSE file for licensing information.

## Contributing

Contributions, bug reports, and suggestions are welcome.

Please include reproducible examples when reporting parsing or serialization issues.
