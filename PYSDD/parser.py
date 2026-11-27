#
#   file: SDD/PYSDD/parser.py
#   
#   Creator: Aleksander Sapieha (sourceguy68@gmail.com)
#
#   Contributors:
#   Aleksander Sapieha (sourceguy68@gmail.com)
#

from decimal import Decimal, InvalidOperation

from .lexer import Lexer
from .errors import (
    SDDSyntaxError,
    SDDTypeError,
    SDDDuplicateKeyError,
    SDDVersionError
)


class Parser:

    def __init__(self, source):
        self.tokens = Lexer(source).tokenize()
        self.position = 0

    def current(self):
        return self.tokens[self.position]

    def advance(self):
        token = self.current()
        self.position += 1
        return token

    def skip_newlines(self):
        while self.current().type == "NEWLINE":
            self.advance()

    def error(self, message):
        token = self.current()

        raise SDDSyntaxError(
            f"{message} at "
            f"{token.line}:{token.column}"
        )

    def expect(self, token_type):
        token = self.current()

        if token.type != token_type:
            self.error(
                f"Expected {token_type}, got {token.type}"
            )

        return self.advance()

    def parse(self):
        self.skip_newlines()

        self.expect("SDD")

        version = self.expect("DECIMAL").value

        if version != "1.0":
            raise SDDVersionError(
                f"Unsupported SDD version: {version}"
            )

        self.skip_newlines()

        values = self.parse_statements()

        self.expect("EOF")

        return {
            "version": version,
            "values": values
        }

    def parse_statements(self, nested=False):
        values = {}

        while True:
            self.skip_newlines()

            token = self.current()

            if token.type == "EOF":
                if nested:
                    self.error("Expected 'end'")
                break

            if token.type == "END":
                if not nested:
                    self.error("Unexpected 'end'")

                self.advance()
                break

            name = self.expect("IDENTIFIER").value

            if name in values:
                raise SDDDuplicateKeyError(
                    f"Duplicate key: {name}"
                )

            self.skip_newlines()

            token = self.current()

            if token.type == "CONTAINS":
                self.advance()

                values[name] = self.parse_statements(
                    nested=True
                )

            elif token.type == "IS":
                self.advance()

                values[name] = self.parse_assignment()

            else:
                self.error("Expected 'is' or 'contains'")

        return values

    def parse_assignment(self):
        type_token = self.advance()

        if type_token.type == "ARRAY_TYPE":
            return self.parse_array()

        return self.parse_typed_value(type_token)


    def parse_typed_value(self, type_token):
        type_name = type_token.type

        if type_name == "STRING_TYPE":
            value_token = self.expect("STRING")
            return value_token.value

        elif type_name == "INTEGER_TYPE":
            value_token = self.expect("INTEGER")
            return int(value_token.value)

        elif type_name == "DECIMAL_TYPE":
            value_token = self.expect("DECIMAL")

            try:
                return Decimal(value_token.value)
            except InvalidOperation:
                self.error("Invalid decimal value")

        elif type_name == "BOOLEAN_TYPE":
            value_token = self.expect("BOOLEAN")
            return value_token.value == "true"

        elif type_name == "NULL_TYPE":
            return None

        else:
            self.error("Unknown or unsupported type")



    def parse_array(self):
        values = []

        self.skip_newlines()

        while self.current().type != "END":

            if self.current().type == "EOF":
                self.error("Expected 'end' to close array")

            # Object inside array
            if self.current().type == "CONTAINS":
                self.advance()
                value = self.parse_statements(nested=True)

            # Nested array
            elif self.current().type == "ARRAY_TYPE":
                self.advance()
                value = self.parse_array()

            # Primitive value
            else:
                type_token = self.advance()
                value = self.parse_typed_value(type_token)

            values.append(value)

            if self.current().type not in ("NEWLINE", "END"):
                self.error("Expected a new line or 'end' after array item")

            self.skip_newlines()

        self.expect("END")
        return values

    def parse_file(self, filepath):
        with open(filepath, "r", encoding="utf-8") as file:
            source = file.read()

        self.__init__(source)
        return self.parse()
