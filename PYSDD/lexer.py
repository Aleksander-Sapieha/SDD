
from dataclasses import dataclass


@dataclass
class Token:
    type: str
    value: object
    line: int
    column: int


class LexerError(Exception):
    pass


class Lexer:
    KEYWORDS = {
        "SDD": "SDD",
        "is": "IS",
        "contains": "CONTAINS",
        "end": "END",
        "type": "TYPE",
        "string": "STRING_TYPE",
        "integer": "INTEGER_TYPE",
        "decimal": "DECIMAL_TYPE",
        "boolean": "BOOLEAN_TYPE",
        "null": "NULL_TYPE",
        "array": "ARRAY_TYPE",
        "true": "BOOLEAN",
        "false": "BOOLEAN",
    }

    def __init__(self, source):
        self.source = source
        self.position = 0
        self.line = 1
        self.column = 1
        self.tokens = []

        if source.startswith("\ufeff"):
            raise LexerError("UTF-8 BOM is not permitted")

    def current(self):
        if self.position >= len(self.source):
            return None

        return self.source[self.position]

    def peek(self, offset=1):
        pos = self.position + offset

        if pos >= len(self.source):
            return None

        return self.source[pos]

    def advance(self):
        char = self.current()

        if char is None:
            return None

        self.position += 1

        if char == "\n":
            self.line += 1
            self.column = 1
        else:
            self.column += 1

        return char

    def add_token(self, token_type, value, line, column):
        self.tokens.append(
            Token(token_type, value, line, column)
        )

    def read_string(self):
        line = self.line
        column = self.column

        self.advance()  # Opening quote

        result = ""

        escapes = {
            '"': '"',
            "\\": "\\",
            "n": "\n",
            "t": "\t"
        }

        while self.current() is not None:
            char = self.advance()

            if char == '"':
                self.add_token(
                    "STRING",
                    result,
                    line,
                    column
                )
                return

            if char in ("\n", "\r"):
                raise LexerError(
                    f"Multiline strings are forbidden "
                    f"at {line}:{column}"
                )

            if char == "\\":
                escaped = self.current()

                if escaped is None:
                    raise LexerError(
                        f"Unterminated escape at {self.line}:{self.column}"
                    )

                if escaped not in escapes:
                    raise LexerError(
                        f"Invalid escape sequence at "
                        f"{self.line}:{self.column}"
                    )

                self.advance()
                result += escapes[escaped]
            else:
                result += char

        raise LexerError(
            f"Unterminated string at {line}:{column}"
        )

    def read_number(self):
        line = self.line
        column = self.column

        result = ""

        if self.current() == "-":
            result += self.advance()

        while self.current() is not None and self.current().isdigit():
            result += self.advance()

        if self.current() == ".":
            result += self.advance()

            if self.current() is None or not self.current().isdigit():
                raise LexerError(
                    f"Invalid decimal at {line}:{column}"
                )

            while self.current() is not None and self.current().isdigit():
                result += self.advance()

            self.add_token(
                "DECIMAL",
                result,
                line,
                column
            )
        else:
            self.add_token(
                "INTEGER",
                result,
                line,
                column
            )

    def read_identifier(self):
        line = self.line
        column = self.column

        result = ""

        while self.current() is not None:
            char = self.current()

            if not (char.isalnum() or char == "_"):
                break

            result += self.advance()

        token_type = self.KEYWORDS.get(
            result,
            "IDENTIFIER"
        )

        self.add_token(
            token_type,
            result,
            line,
            column
        )

    def tokenize(self):
        while self.current() is not None:
            char = self.current()

            if char in " \t":
                self.advance()
                continue

            if char in "\r\n":
                line = self.line
                column = self.column

                if char == "\r":
                    self.advance()

                    if self.current() == "\n":
                        self.advance()
                else:
                    self.advance()

                self.add_token(
                    "NEWLINE",
                    "\n",
                    line,
                    column
                )
                continue

            if char == "#":
                while self.current() not in (None, "\n", "\r"):
                    self.advance()
                continue

            if char == '"':
                self.read_string()
                continue

            if char == "-" or char.isdigit():
                self.read_number()
                continue

            if char.isalpha() or char == "_":
                self.read_identifier()
                continue

            raise LexerError(
                f"Unexpected character {char!r} "
                f"at {self.line}:{self.column}"
            )

        self.add_token(
            "EOF",
            None,
            self.line,
            self.column
        )

        return self.tokens
