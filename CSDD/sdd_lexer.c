/*
**  File: SDD/CDD/sdd_lexer.c
**
**  Creator: Aleksander Sapieha (sourceguy68@gmail.com)
**
**  Contributors:
**  Aleksander Sapieha (sourceguy68@gmail.com)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "sdd_lexer.h"

const Keyword KEYWORDS[] = {
    {"SDD", "SDD"},
    {"is", "IS"},
    {"contains", "CONTAINS"},
    {"end", "END"},
    {"type", "TYPE"},
    {"string", "STRING_TYPE"},
    {"integer", "INTEGER_TYPE"},
    {"decimal", "DECIMAL_TYPE"},
    {"boolean", "BOOLEAN_TYPE"},
    {"null", "NULL_TYPE"},
    {"array", "ARRAY_TYPE"},
    {"true", "BOOLEAN"},
    {"false", "BOOLEAN"}
};

const int KEYWORDS_COUNT =
    sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);


void buffer_init(StringBuffer* buffer) {
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

int buffer_push(StringBuffer* buffer, char character) {
    if (buffer->length + 1 >= buffer->capacity) {
        size_t new_capacity =
            buffer->capacity == 0 ? 16 : buffer->capacity * 2;

        char* new_data = realloc(
            buffer->data,
            new_capacity
        );

        if (new_data == NULL)
            return 0;

        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }

    buffer->data[buffer->length++] = character;
    return 1;
}

char* buffer_finish(StringBuffer* buffer) {
    if (!buffer_push(buffer, '\0'))
        return NULL;

    return buffer->data;
}

void lexer_error(
    Lexer* self,
    const char* format,
    ...
) {
    va_list args;
    va_start(args, format);

    vsnprintf(
        self->error.message,
        sizeof(self->error.message),
        format,
        args
    );

    va_end(args);
}

char* string_copy(const char* source) {
    if (source == NULL)
        return NULL;

    size_t length = strlen(source) + 1;

    char* result = malloc(length);

    if (result == NULL)
        return NULL;

    memcpy(result, source, length);

    return result;
}

void lexer_destroy(Lexer* self) {
    for (size_t i = 0; i < self->token_count; i++) {
        free(self->tokens[i].value);
    }

    free(self->tokens);

    self->tokens = NULL;
    self->token_count = 0;
    self->token_capacity = 0;
}


int lexer_init(Lexer* self, const char* source) {
    memset(self, 0, sizeof(Lexer));

    self->source = source;
    self->position = 0;
    self->line = 1;
    self->column = 1;

    if (
        (unsigned char)source[0] == 0xEF &&
        (unsigned char)source[1] == 0xBB &&
        (unsigned char)source[2] == 0xBF
    ) {
        lexer_error(
            self,
            "UTF-8 BOM is not permitted"
        );

        return 0;
    }

    return 1;
}


int lexer_current(Lexer* self) {
    if (self->source[self->position] == '\0')
        return EOF;

    return (unsigned char)self->source[self->position];
}

int lexer_peek(Lexer* self, size_t offset) {
    size_t pos = self->position + offset;

    if (self->source[pos] == '\0')
        return EOF;

    return (unsigned char)self->source[pos];
}

int lexer_advance(Lexer* self) {
    int character = lexer_current(self);

    if (character == EOF)
        return EOF;

    self->position++;

    if (character == '\n') {
        self->line++;
        self->column = 1;
    } else {
        self->column++;
    }

    return character;
}


int lexer_add_token(
    Lexer* self,
    const char* token_type,
    const char* value,
    int line,
    int column
) {
    if (self->token_count >= self->token_capacity) {
        size_t new_capacity =
            self->token_capacity == 0
            ? 16
            : self->token_capacity * 2;

        Token* new_tokens = realloc(
            self->tokens,
            new_capacity * sizeof(Token)
        );

        if (new_tokens == NULL) {
            lexer_error(self, "Out of memory");
            return 0;
        }

        self->tokens = new_tokens;
        self->token_capacity = new_capacity;
    }

    char* copied_value = string_copy(value);

    if (value != NULL && copied_value == NULL) {
        lexer_error(self, "Out of memory");
        return 0;
    }

    self->tokens[self->token_count++] = (Token) {
        .type = token_type,
        .value = copied_value,
        .line = line,
        .column = column
    };

    return 1;
}

const char* lexer_get_keyword(const char* key) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(KEYWORDS[i].key, key) == 0)
            return KEYWORDS[i].value;
    }

    return "IDENTIFIER";
}


int lexer_read_string(Lexer* self) {
    int line = self->line;
    int column = self->column;

    lexer_advance(self); /* Opening quote */

    StringBuffer result;
    buffer_init(&result);

    while (lexer_current(self) != EOF) {
        int character = lexer_advance(self);

        if (character == '"') {
            char* value = buffer_finish(&result);

            if (value == NULL) {
                free(result.data);
                lexer_error(self, "Out of memory");
                return 0;
            }

            int success = lexer_add_token(
                self,
                "STRING",
                value,
                line,
                column
            );

            free(value);
            return success;
        }

        if (character == '\n' || character == '\r') {
            free(result.data);

            lexer_error(
                self,
                "Multiline strings are forbidden at %d:%d",
                line,
                column
            );

            return 0;
        }

        if (character == '\\') {
            int escaped = lexer_current(self);

            if (escaped == EOF) {
                free(result.data);

                lexer_error(
                    self,
                    "Unterminated escape at %d:%d",
                    self->line,
                    self->column
                );

                return 0;
            }

            char replacement;

            switch (escaped) {
                case '"':  replacement = '"';  break;
                case '\\': replacement = '\\'; break;
                case 'n':  replacement = '\n'; break;
                case 't':  replacement = '\t'; break;

                default:
                    free(result.data);

                    lexer_error(
                        self,
                        "Invalid escape sequence at %d:%d",
                        self->line,
                        self->column
                    );

                    return 0;
            }

            lexer_advance(self);

            if (!buffer_push(&result, replacement)) {
                free(result.data);
                lexer_error(self, "Out of memory");
                return 0;
            }
        } else {
            if (!buffer_push(&result, (char)character)) {
                free(result.data);
                lexer_error(self, "Out of memory");
                return 0;
            }
        }
    }

    free(result.data);

    lexer_error(
        self,
        "Unterminated string at %d:%d",
        line,
        column
    );

    return 0;
}


int lexer_read_number(Lexer* self) {
    int line = self->line;
    int column = self->column;

    StringBuffer result;
    buffer_init(&result);

    if (lexer_current(self) == '-') {
        if (!buffer_push(&result, (char)lexer_advance(self)))
            goto memory_error;
    }

    while (isdigit(lexer_current(self))) {
        if (!buffer_push(&result, (char)lexer_advance(self)))
            goto memory_error;
    }

    const char* token_type = "INTEGER";

    if (lexer_current(self) == '.') {
        if (!buffer_push(&result, (char)lexer_advance(self)))
            goto memory_error;

        if (
            lexer_current(self) == EOF ||
            !isdigit(lexer_current(self))
        ) {
            free(result.data);

            lexer_error(
                self,
                "Invalid decimal at %d:%d",
                line,
                column
            );

            return 0;
        }

        while (isdigit(lexer_current(self))) {
            if (!buffer_push(&result, (char)lexer_advance(self)))
                goto memory_error;
        }

        token_type = "DECIMAL";
    }

    {
        char* value = buffer_finish(&result);

        if (value == NULL)
            goto memory_error;

        int success = lexer_add_token(
            self,
            token_type,
            value,
            line,
            column
        );

        free(value);
        return success;
    }

memory_error:
    free(result.data);
    lexer_error(self, "Out of memory");
    return 0;
}


int lexer_read_identifier(Lexer* self) {
    int line = self->line;
    int column = self->column;

    StringBuffer result;
    buffer_init(&result);

    while (lexer_current(self) != EOF) {
        int character = lexer_current(self);

        if (
            !(isalnum(character) || character == '_')
        )
            break;

        if (!buffer_push(&result, (char)lexer_advance(self))) {
            free(result.data);
            lexer_error(self, "Out of memory");
            return 0;
        }
    }

    char* value = buffer_finish(&result);

    if (value == NULL) {
        free(result.data);
        lexer_error(self, "Out of memory");
        return 0;
    }

    const char* token_type = lexer_get_keyword(value);

    int success = lexer_add_token(
        self,
        token_type,
        value,
        line,
        column
    );

    free(value);
    return success;
}

int lexer_tokenize(Lexer* self) {
    while (lexer_current(self) != EOF) {
        int character = lexer_current(self);

        /* Whitespace */

        if (character == ' ' || character == '\t') {
            lexer_advance(self);
            continue;
        }

        /* Newlines */

        if (character == '\r' || character == '\n') {
            int line = self->line;
            int column = self->column;

            if (character == '\r') {
                lexer_advance(self);

                if (lexer_current(self) == '\n')
                    lexer_advance(self);
            } else {
                lexer_advance(self);
            }

            if (!lexer_add_token(
                self,
                "NEWLINE",
                "\n",
                line,
                column
            ))
                return 0;

            continue;
        }

        /* Comments */

        if (character == '#') {
            while (
                lexer_current(self) != EOF &&
                lexer_current(self) != '\n' &&
                lexer_current(self) != '\r'
            ) {
                lexer_advance(self);
            }

            continue;
        }

        /* Strings */

        if (character == '"') {
            if (!lexer_read_string(self))
                return 0;

            continue;
        }

        /* Numbers */

        if (character == '-' || isdigit(character)) {
            if (!lexer_read_number(self))
                return 0;

            continue;
        }

        /* Identifiers */

        if (isalpha(character) || character == '_') {
            if (!lexer_read_identifier(self))
                return 0;

            continue;
        }

        /* Unexpected character */

        lexer_error(
            self,
            "Unexpected character '%c' at %d:%d",
            character,
            self->line,
            self->column
        );

        return 0;
    }

    /* EOF */

    if (!lexer_add_token(
        self,
        "EOF",
        NULL,
        self->line,
        self->column
    ))
        return 0;

    return 1;
}
