from .parser import Parser


def loads(source):
    """Parse SDD source text."""
    return Parser(source).parse()


def load(filepath):
    """Load and parse an SDD file."""
    with open(filepath, "r", encoding="utf-8") as file:
        source = file.read()

    return loads(source)
