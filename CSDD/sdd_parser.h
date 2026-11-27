/*
**  File: SDD/CSDD/sdd_parser.h
**
**  Creator: Aleksander Sapieha (sourceguy68@gmail.com)
**
**  Contributors:
**  Aleksander Sapieha (sourceguy68@gmail.com)
*/

#ifndef _SDD_PARSER_H
#define _SDD_PARSER_H

#include <stdlib.h>
#include <stdint.h>

#include "sdd_lexer.h"


typedef enum {
    VALUE_STRING,
    VALUE_INTEGER,
    VALUE_DECIMAL,
    VALUE_BOOLEAN,
    VALUE_NULL,
    VALUE_ARRAY,
    VALUE_OBJECT
} ValueType;

typedef struct Value Value;

typedef struct {
    char* key;
    Value* value;
} ObjectEntry;

typedef struct {
    ObjectEntry* entries;
    size_t count;
    size_t capacity;
} Object;

typedef struct {
    Value* items;
    size_t count;
    size_t capacity;
} Array;

struct Value {
    ValueType type;

    union {
        char* string;
        int64_t integer;
        char* decimal;
        int boolean;
        Array array;
        Object object;
    } data;
};

typedef struct {
    char* version;
    Value values;
} SDDDocument;

typedef enum {
    SDD_ERROR_NONE,
    SDD_SYNTAX_ERROR,
    SDD_TYPE_ERROR,
    SDD_DUPLICATE_KEY_ERROR,
    SDD_VERSION_ERROR,
    SDD_MEMORY_ERROR,
    SDD_FILE_ERROR
} SDDErrorType;

typedef struct {
    SDDErrorType type;
    char message[512];
} SDDError;

typedef struct {
    Token* tokens;
    size_t token_count;
    size_t position;

    Lexer lexer;
    SDDError error;
} Parser;

static char* copy_string(const char* source);

Value value_null(void);

Value value_string(const char* string);

Value value_integer(int64_t integer);

Value value_decimal(const char* decimal);

Value value_boolean(int boolean);

Value value_array(void);

Value value_object(void);

void value_destroy(Value* value);

int array_append(Array* array, Value value);

int object_has(Object* object, const char* key);

int object_set(Object* object, const char* key, Value value);

void parser_error(
    Parser* self,
    SDDErrorType type,
    const char* message
);

int parser_init(Parser* self, const char* source);

void parser_destroy(Parser* self);

Token* parser_current(Parser* self);

Token* parser_advance(Parser* self);

void parser_skip_newlines(Parser* self);

Token* parser_expect(Parser* self, const char* token_type);

int parser_parse_statements(
    Parser* self,
    Value* result,
    int nested
);

int parser_parse_array(
    Parser* self,
    Value* result
);

int parser_parse_typed_value(
    Parser* self,
    Token* type_token,
    Value* result
);

int parser_parse_assignment(Parser* self, Value* result);


int parser_parse(Parser* self, SDDDocument* document);

void sdd_document_destroy(SDDDocument* document);

int parser_parse_file(
    Parser* self,
    const char* filepath,
    SDDDocument* document
);

#endif // _SDD_PARSER_H