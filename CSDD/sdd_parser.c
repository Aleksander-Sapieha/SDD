/*
**  File: SDD/CSDD/sdd_parser.c
**
**  Creator: Aleksander Sapieha (sourceguy68@gmail.com)
**
**  Contributors:
**  Aleksander Sapieha (sourceguy68@gmail.com)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdint.h>

#include "sdd_lexer.h"
#include "sdd_parser.h"


static char* copy_string(const char* source) {
    if (source == NULL)
        return NULL;

    size_t length = strlen(source) + 1;

    char* result = malloc(length);

    if (result == NULL)
        return NULL;

    memcpy(result, source, length);

    return result;
}

Value value_null(void) {
    return (Value) {
        .type = VALUE_NULL
    };
}

Value value_string(const char* string) {
    Value value = {
        .type = VALUE_STRING
    };

    value.data.string = copy_string(string);

    return value;
}

Value value_integer(int64_t integer) {
    Value value = {
        .type = VALUE_INTEGER
    };

    value.data.integer = integer;

    return value;
}

Value value_decimal(const char* decimal) {
    Value value = {
        .type = VALUE_DECIMAL
    };

    value.data.decimal = copy_string(decimal);

    return value;
}

Value value_boolean(int boolean) {
    Value value = {
        .type = VALUE_BOOLEAN
    };

    value.data.boolean = boolean;

    return value;
}

Value value_array(void) {
    Value value = {
        .type = VALUE_ARRAY
    };

    value.data.array = (Array) {0};

    return value;
}

Value value_object(void) {
    Value value = {
        .type = VALUE_OBJECT
    };

    value.data.object = (Object) {0};

    return value;
}

void value_destroy(Value* value) {
    if (value == NULL)
        return;

    switch (value->type) {
        case VALUE_STRING:
            free(value->data.string);
            break;

        case VALUE_DECIMAL:
            free(value->data.decimal);
            break;

        case VALUE_ARRAY:
            for (size_t i = 0; i < value->data.array.count; i++) {
                value_destroy(&value->data.array.items[i]);
            }

            free(value->data.array.items);
            break;

        case VALUE_OBJECT:
            for (size_t i = 0; i < value->data.object.count; i++) {
                free(value->data.object.entries[i].key);

                value_destroy(
                    value->data.object.entries[i].value
                );

                free(value->data.object.entries[i].value);
            }

            free(value->data.object.entries);
            break;

        default:
            break;
    }

    *value = value_null();
}

int array_append(Array* array, Value value) {
    if (array->count >= array->capacity) {
        size_t capacity =
            array->capacity == 0
            ? 8
            : array->capacity * 2;

        Value* items = realloc(
            array->items,
            capacity * sizeof(Value)
        );

        if (items == NULL)
            return 0;

        array->items = items;
        array->capacity = capacity;
    }

    array->items[array->count++] = value;

    return 1;
}


int object_has(Object* object, const char* key) {
    for (size_t i = 0; i < object->count; i++) {
        if (strcmp(object->entries[i].key, key) == 0)
            return 1;
    }

    return 0;
}

int object_set(Object* object, const char* key, Value value) {
    if (object_has(object, key))
        return 0;

    if (object->count >= object->capacity) {
        size_t capacity =
            object->capacity == 0
            ? 8
            : object->capacity * 2;

        ObjectEntry* entries = realloc(
            object->entries,
            capacity * sizeof(ObjectEntry)
        );

        if (entries == NULL)
            return 0;

        object->entries = entries;
        object->capacity = capacity;
    }

    char* copied_key = copy_string(key);
    Value* stored_value = malloc(sizeof(Value));

    if (copied_key == NULL || stored_value == NULL) {
        free(copied_key);
        free(stored_value);
        return 0;
    }

    *stored_value = value;

    object->entries[object->count++] = (ObjectEntry) {
        .key = copied_key,
        .value = stored_value
    };

    return 1;
}

void parser_error(
    Parser* self,
    SDDErrorType type,
    const char* message
) {
    Token* token = &self->tokens[self->position];

    self->error.type = type;

    snprintf(
        self->error.message,
        sizeof(self->error.message),
        "%s at %d:%d",
        message,
        token->line,
        token->column
    );
}

int parser_init(Parser* self, const char* source) {
    memset(self, 0, sizeof(Parser));

    if (!lexer_init(&self->lexer, source)) {
        self->error.type = SDD_SYNTAX_ERROR;

        snprintf(
            self->error.message,
            sizeof(self->error.message),
            "%s",
            self->lexer.error.message
        );

        return 0;
    }

    if (!lexer_tokenize(&self->lexer)) {
        self->error.type = SDD_SYNTAX_ERROR;

        snprintf(
            self->error.message,
            sizeof(self->error.message),
            "%s",
            self->lexer.error.message
        );

        lexer_destroy(&self->lexer);
        return 0;
    }

    self->tokens = self->lexer.tokens;
    self->token_count = self->lexer.token_count;
    self->position = 0;

    return 1;
}

void parser_destroy(Parser* self) {
    lexer_destroy(&self->lexer);

    self->tokens = NULL;
    self->token_count = 0;
}

Token* parser_current(Parser* self) {
    if (self->position >= self->token_count)
        return NULL;

    return &self->tokens[self->position];
}

Token* parser_advance(Parser* self) {
    Token* token = parser_current(self);

    if (token != NULL)
        self->position++;

    return token;
}

void parser_skip_newlines(Parser* self) {
    while (
        parser_current(self) != NULL &&
        strcmp(parser_current(self)->type, "NEWLINE") == 0
    ) {
        parser_advance(self);
    }
}

Token* parser_expect(Parser* self, const char* token_type) {
    Token* token = parser_current(self);

    if (
        token == NULL ||
        strcmp(token->type, token_type) != 0
    ) {
        char message[256];

        snprintf(
            message,
            sizeof(message),
            "Expected %s, got %s",
            token_type,
            token != NULL ? token->type : "EOF"
        );

        parser_error(self, SDD_SYNTAX_ERROR, message);

        return NULL;
    }

    return parser_advance(self);
}

/* =========================
   PARSE ASSIGNMENT
   ========================= */

int parser_parse_assignment(Parser* self, Value* result) {
    Token* type_token = parser_advance(self);

    if (type_token == NULL) {
        parser_error(
            self,
            SDD_SYNTAX_ERROR,
            "Expected a value type"
        );

        return 0;
    }

    if (strcmp(type_token->type, "ARRAY_TYPE") == 0) {
        return parser_parse_array(self, result);
    }

    return parser_parse_typed_value(
        self,
        type_token,
        result
    );
}


/* =========================
   PARSE TYPED VALUE
   ========================= */

int parser_parse_typed_value(
    Parser* self,
    Token* type_token,
    Value* result
) {
    const char* type = type_token->type;

    if (strcmp(type, "STRING_TYPE") == 0) {
        Token* token = parser_expect(self, "STRING");

        if (token == NULL)
            return 0;

        *result = value_string(token->value);

        if (result->data.string == NULL) {
            parser_error(self, SDD_MEMORY_ERROR, "Out of memory");
            return 0;
        }

        return 1;
    }

    if (strcmp(type, "INTEGER_TYPE") == 0) {
        Token* token = parser_expect(self, "INTEGER");

        if (token == NULL)
            return 0;

        errno = 0;
        char* end;

        long long number = strtoll(token->value, &end, 10);

        if (errno == ERANGE || *end != '\0') {
            parser_error(
                self,
                SDD_TYPE_ERROR,
                "Invalid integer value"
            );

            return 0;
        }

        *result = value_integer((int64_t)number);

        return 1;
    }

    if (strcmp(type, "DECIMAL_TYPE") == 0) {
        Token* token = parser_expect(self, "DECIMAL");

        if (token == NULL)
            return 0;

        *result = value_decimal(token->value);

        if (result->data.decimal == NULL) {
            parser_error(self, SDD_MEMORY_ERROR, "Out of memory");
            return 0;
        }

        return 1;
    }

    if (strcmp(type, "BOOLEAN_TYPE") == 0) {
        Token* token = parser_expect(self, "BOOLEAN");

        if (token == NULL)
            return 0;

        *result = value_boolean(
            strcmp(token->value, "true") == 0
        );

        return 1;
    }

    if (strcmp(type, "NULL_TYPE") == 0) {
        *result = value_null();
        return 1;
    }

    parser_error(
        self,
        SDD_TYPE_ERROR,
        "Unknown or unsupported type"
    );

    return 0;
}

int parser_parse_array(Parser* self, Value* result) {
    *result = value_array();

    parser_skip_newlines(self);

    while (1) {
        Token* token = parser_current(self);

        if (token == NULL) {
            parser_error(
                self,
                SDD_SYNTAX_ERROR,
                "Expected 'end' to close array"
            );

            goto failure;
        }

        if (strcmp(token->type, "END") == 0)
            break;

        if (strcmp(token->type, "EOF") == 0) {
            parser_error(
                self,
                SDD_SYNTAX_ERROR,
                "Expected 'end' to close array"
            );

            goto failure;
        }

        Value value;

        /* Object inside array */

        if (strcmp(token->type, "CONTAINS") == 0) {
            parser_advance(self);

            if (!parser_parse_statements(self, &value, 1))
                goto failure;
        }

        /* Nested array */

        else if (strcmp(token->type, "ARRAY_TYPE") == 0) {
            parser_advance(self);

            if (!parser_parse_array(self, &value))
                goto failure;
        }

        /* Primitive value */

        else {
            Token* type_token = parser_advance(self);

            if (!parser_parse_typed_value(
                self,
                type_token,
                &value
            )) {
                goto failure;
            }
        }

        if (!array_append(&result->data.array, value)) {
            value_destroy(&value);

            parser_error(
                self,
                SDD_MEMORY_ERROR,
                "Out of memory"
            );

            goto failure;
        }

        token = parser_current(self);

        if (
            token == NULL ||
            (
                strcmp(token->type, "NEWLINE") != 0 &&
                strcmp(token->type, "END") != 0
            )
        ) {
            parser_error(
                self,
                SDD_SYNTAX_ERROR,
                "Expected a new line or 'end' after array item"
            );

            goto failure;
        }

        parser_skip_newlines(self);
    }

    if (parser_expect(self, "END") == NULL)
        goto failure;

    return 1;

failure:
    value_destroy(result);
    return 0;
}

int parser_parse_statements(
    Parser* self,
    Value* result,
    int nested
) {
    *result = value_object();

    while (1) {
        parser_skip_newlines(self);

        Token* token = parser_current(self);

        if (token == NULL) {
            parser_error(
                self,
                SDD_SYNTAX_ERROR,
                "Unexpected end of input"
            );

            goto failure;
        }

        if (strcmp(token->type, "EOF") == 0) {
            if (nested) {
                parser_error(
                    self,
                    SDD_SYNTAX_ERROR,
                    "Expected 'end'"
                );

                goto failure;
            }

            break;
        }

        if (strcmp(token->type, "END") == 0) {
            if (!nested) {
                parser_error(
                    self,
                    SDD_SYNTAX_ERROR,
                    "Unexpected 'end'"
                );

                goto failure;
            }

            parser_advance(self);
            break;
        }

        /* Read identifier */

        Token* name_token = parser_expect(self, "IDENTIFIER");

        if (name_token == NULL)
            goto failure;

        if (object_has(&result->data.object, name_token->value)) {
            char message[256];

            snprintf(
                message,
                sizeof(message),
                "Duplicate key: %s",
                name_token->value
            );

            parser_error(
                self,
                SDD_DUPLICATE_KEY_ERROR,
                message
            );

            goto failure;
        }

        parser_skip_newlines(self);

        token = parser_current(self);

        if (token == NULL) {
            parser_error(
                self,
                SDD_SYNTAX_ERROR,
                "Expected 'is' or 'contains'"
            );

            goto failure;
        }

        Value value;

        /* Nested object */

        if (strcmp(token->type, "CONTAINS") == 0) {
            parser_advance(self);

            if (!parser_parse_statements(self, &value, 1))
                goto failure;
        }

        /* Assignment */

        else if (strcmp(token->type, "IS") == 0) {
            parser_advance(self);

            if (!parser_parse_assignment(self, &value))
                goto failure;
        }

        else {
            parser_error(
                self,
                SDD_SYNTAX_ERROR,
                "Expected 'is' or 'contains'"
            );

            goto failure;
        }

        if (!object_set(
            &result->data.object,
            name_token->value,
            value
        )) {
            value_destroy(&value);

            parser_error(
                self,
                SDD_MEMORY_ERROR,
                "Out of memory or duplicate key"
            );

            goto failure;
        }
    }

    return 1;

failure:
    value_destroy(result);
    return 0;
}

int parser_parse(Parser* self, SDDDocument* document) {
    memset(document, 0, sizeof(SDDDocument));

    parser_skip_newlines(self);

    if (parser_expect(self, "SDD") == NULL)
        return 0;

    Token* version = parser_expect(self, "DECIMAL");

    if (version == NULL)
        return 0;

    if (strcmp(version->value, "1.0") != 0) {
        self->error.type = SDD_VERSION_ERROR;

        snprintf(
            self->error.message,
            sizeof(self->error.message),
            "Unsupported SDD version: %s",
            version->value
        );

        return 0;
    }

    document->version = copy_string(version->value);

    if (document->version == NULL) {
        self->error.type = SDD_MEMORY_ERROR;
        snprintf(self->error.message, sizeof(self->error.message),
                 "Out of memory");
        return 0;
    }

    parser_skip_newlines(self);

    if (!parser_parse_statements(self, &document->values, 0)) {
        free(document->version);
        document->version = NULL;
        return 0;
    }

    if (parser_expect(self, "EOF") == NULL) {
        free(document->version);
        document->version = NULL;
        value_destroy(&document->values);
        return 0;
    }

    return 1;
}

void sdd_document_destroy(SDDDocument* document) {
    free(document->version);
    document->version = NULL;

    value_destroy(&document->values);
}


int parser_parse_file(
    Parser* self,
    const char* filepath,
    SDDDocument* document
) {
    FILE* file = fopen(filepath, "rb");

    if (file == NULL) {
        self->error.type = SDD_FILE_ERROR;

        snprintf(
            self->error.message,
            sizeof(self->error.message),
            "Could not open file: %s",
            filepath
        );

        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        self->error.type = SDD_FILE_ERROR;
        snprintf(self->error.message, sizeof(self->error.message),
                 "Could not read file");
        return 0;
    }

    long size = ftell(file);

    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        self->error.type = SDD_FILE_ERROR;
        snprintf(self->error.message, sizeof(self->error.message),
                 "Could not read file");
        return 0;
    }

    char* source = malloc((size_t)size + 1);

    if (source == NULL) {
        fclose(file);
        self->error.type = SDD_MEMORY_ERROR;
        snprintf(self->error.message, sizeof(self->error.message),
                 "Out of memory");
        return 0;
    }

    size_t bytes_read = fread(source, 1, (size_t)size, file);

    if (bytes_read != (size_t)size || ferror(file)) {
        free(source);
        fclose(file);
        self->error.type = SDD_FILE_ERROR;
        snprintf(self->error.message, sizeof(self->error.message),
                 "Could not read file");
        return 0;
    }

    source[size] = '\0';
    fclose(file);

    int success = parser_init(self, source);

    free(source);

    if (!success)
        return 0;

    return parser_parse(self, document);
}