/*
**  File: SDD/CSDD/sdd_lexer.h
**
**  Creator: Aleksander Sapieha (sourceguy68@gmail.com)
**
**  Contributors:
**  Aleksander Sapieha (sourceguy68@gmail.com)
*/

#ifndef _SDD_LEXER_H
#define _SDD_LEXER_H

#include <stdio.h>
#include <string.h>


typedef struct {
    const char* type;
    char* value;
    int line;
    int column;
} Token;

typedef struct {
    char message[256];
} LexerError;


typedef struct {
    const char* key;
    const char* value;
} Keyword;


typedef struct {
    const char* source;
    size_t position;

    int line;
    int column;

    Token* tokens;
    size_t token_count;
    size_t token_capacity;

    LexerError error;
} Lexer;

extern const Keyword KEYWORDS[];
extern const int KEYWORDS_COUNT;

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} StringBuffer;

void buffer_init(StringBuffer* buffer);

int buffer_push(StringBuffer* buffer, char character);

void lexer_error(
    Lexer* self,
    const char* format,
    ...
);

char* string_copy(const char* source);

void lexer_destroy(Lexer* self);

int lexer_init(Lexer* self, const char* source);

int lexer_current(Lexer* self);

int lexer_peek(Lexer* self, size_t offset);

int lexer_advance(Lexer* self);

int lexer_add_token(
    Lexer* self,
    const char* token_type,
    const char* value,
    int line,
    int column
);

const char* lexer_get_keyword(const char* key);

int lexer_read_string(Lexer* self);

int lexer_read_number(Lexer* self);

int lexer_read_identifier(Lexer* self);

int lexer_tokenize(Lexer* self);

#endif // _SDD_LEXER