/*
**  File: SDD/CSDD/sdd.c
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
#include <stdint.h>
#include <stdarg.h>

#include "sdd.h"

static int has_extension(
    const char* filepath,
    const char* extension
) {
    size_t path_length = strlen(filepath);
    size_t ext_length = strlen(extension);

    if (path_length < ext_length)
        return 0;

    const char* suffix = filepath + path_length - ext_length;

    for (size_t i = 0; i < ext_length; i++) {
        if (tolower((unsigned char)suffix[i]) !=
            tolower((unsigned char)extension[i])) {
            return 0;
        }
    }

    return 1;
}

static void set_error(
    SDDError* error,
    SDDErrorType type,
    const char* message
) {
    error->type = type;

    snprintf(
        error->message,
        sizeof(error->message),
        "%s",
        message
    );
}

int sdd_loads(
    const char* source,
    SDDDocument* document,
    SDDError* error
) {
    Parser parser;

    if (!parser_init(&parser, source)) {
        *error = parser.error;
        return 0;
    }

    int success = parser_parse(&parser, document);

    if (!success)
        *error = parser.error;
    else
        error->type = SDD_ERROR_NONE;

    parser_destroy(&parser);

    return success;
}

int sdd_load(
    const char* filepath,
    SDDDocument* document,
    SDDError* error
) {
    if (!has_extension(filepath, ".sdd")) {
        set_error(
            error,
            SDD_FILE_ERROR,
            "Please use a .sdd file extension"
        );

        return 0;
    }

    Parser parser;

    if (!parser_parse_file(&parser, filepath, document)) {
        *error = parser.error;
        parser_destroy(&parser);
        return 0;
    }

    error->type = SDD_ERROR_NONE;

    parser_destroy(&parser);

    return 1;
}

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} StringBuilder;

static int sb_init(StringBuilder* sb) {
    sb->capacity = 128;
    sb->length = 0;
    sb->data = malloc(sb->capacity);

    if (!sb->data)
        return 0;

    sb->data[0] = '\0';
    return 1;
}

static int sb_reserve(StringBuilder* sb, size_t extra) {
    if (extra > SIZE_MAX - sb->length - 1)
        return 0;

    size_t required = sb->length + extra + 1;

    if (required <= sb->capacity)
        return 1;

    size_t capacity = sb->capacity;

    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }

        capacity *= 2;
    }

    char* new_data = realloc(sb->data, capacity);

    if (!new_data)
        return 0;

    sb->data = new_data;
    sb->capacity = capacity;

    return 1;
}

static int sb_append_n(
    StringBuilder* sb,
    const char* text,
    size_t length
) {
    if (!sb_reserve(sb, length))
        return 0;

    memcpy(sb->data + sb->length, text, length);
    sb->length += length;
    sb->data[sb->length] = '\0';

    return 1;
}

static int sb_append(StringBuilder* sb, const char* text) {
    return sb_append_n(sb, text, strlen(text));
}

static int sb_append_char(StringBuilder* sb, char c) {
    return sb_append_n(sb, &c, 1);
}

static int sb_indent(StringBuilder* sb, int indent) {
    for (int i = 0; i < indent; i++) {
        if (!sb_append_char(sb, ' '))
            return 0;
    }

    return 1;
}

static void serializer_error(
    SDDError* error,
    SDDErrorType type,
    const char* message
) {
    if (!error)
        return;

    error->type = type;

    snprintf(
        error->message,
        sizeof(error->message),
        "%s",
        message
    );
}

static int serialize_value(
    StringBuilder* sb,
    const Value* value,
    int indent
);

static int serialize_assignment(
    StringBuilder* sb,
    const char* name,
    const Value* value,
    int indent
);

static int serialize_string(
    StringBuilder* sb,
    const char* value
) {
    if (!sb_append(sb, "string \""))
        return 0;

    for (size_t i = 0; value[i] != '\0'; i++) {
        switch (value[i]) {
            case '\\':
                if (!sb_append(sb, "\\\\"))
                    return 0;
                break;

            case '"':
                if (!sb_append(sb, "\\\""))
                    return 0;
                break;

            case '\n':
                if (!sb_append(sb, "\\n"))
                    return 0;
                break;

            case '\t':
                if (!sb_append(sb, "\\t"))
                    return 0;
                break;

            default:
                if (!sb_append_char(sb, value[i]))
                    return 0;
        }
    }

    return sb_append_char(sb, '"');
}

static int serialize_object_contents(
    StringBuilder* sb,
    const Object* object,
    int indent
) {
    if (!sb_append(sb, "contains\n"))
        return 0;

    for (size_t i = 0; i < object->count; i++) {
        const ObjectEntry* entry = &object->entries[i];

        if (!entry->key || !entry->value)
            return 0;

        if (!serialize_assignment(
            sb,
            entry->key,
            entry->value,
            indent + 4
        ))
            return 0;

        if (!sb_append_char(sb, '\n'))
            return 0;
    }

    if (!sb_indent(sb, indent))
        return 0;

    return sb_append(sb, "end");
}

static int serialize_array(
    StringBuilder* sb,
    const Array* array,
    int indent
) {
    if (!sb_append(sb, "array\n"))
        return 0;

    for (size_t i = 0; i < array->count; i++) {
        if (!sb_indent(sb, indent + 4))
            return 0;

        if (!serialize_value(
            sb,
            &array->items[i],
            indent + 4
        ))
            return 0;

        if (!sb_append_char(sb, '\n'))
            return 0;
    }

    if (!sb_indent(sb, indent))
        return 0;

    return sb_append(sb, "end");
}

static int serialize_value(
    StringBuilder* sb,
    const Value* value,
    int indent
) {
    if (!value)
        return 0;

    switch (value->type) {
        case VALUE_STRING:
            if (!sb_indent(sb, indent))
                return 0;

            return serialize_string(
                sb,
                value->data.string
            );

        case VALUE_INTEGER: {
            char buffer[64];

            snprintf(
                buffer,
                sizeof(buffer),
                "integer %lld",
                (long long)value->data.integer
            );

            return sb_indent(sb, indent)
                && sb_append(sb, buffer);
        }

        case VALUE_DECIMAL:
            return sb_indent(sb, indent)
                && sb_append(sb, "decimal ")
                && sb_append(sb, value->data.decimal);

        case VALUE_BOOLEAN:
            return sb_indent(sb, indent)
                && sb_append(
                    sb,
                    value->data.boolean
                        ? "boolean true"
                        : "boolean false"
                );

        case VALUE_NULL:
            return sb_indent(sb, indent)
                && sb_append(sb, "null");

        case VALUE_ARRAY:
            return sb_indent(sb, indent)
                && serialize_array(
                    sb,
                    &value->data.array,
                    indent
                );

        case VALUE_OBJECT:
            return sb_indent(sb, indent)
                && serialize_object_contents(
                    sb,
                    &value->data.object,
                    indent
                );

        default:
            return 0;
    }
}

/* -------------------------
   Assignment serialization
------------------------- */

static int serialize_assignment(
    StringBuilder* sb,
    const char* name,
    const Value* value,
    int indent
) {
    if (!name || !value)
        return 0;

    if (!sb_indent(sb, indent))
        return 0;

    if (value->type == VALUE_OBJECT) {
        if (!sb_append(sb, name))
            return 0;

        if (!sb_append(sb, " contains\n"))
            return 0;

        const Object* object = &value->data.object;

        for (size_t i = 0; i < object->count; i++) {
            const ObjectEntry* entry = &object->entries[i];

            if (!entry->key || !entry->value)
                return 0;

            if (!serialize_assignment(
                sb,
                entry->key,
                entry->value,
                indent + 4
            ))
                return 0;

            if (!sb_append_char(sb, '\n'))
                return 0;
        }

        if (!sb_indent(sb, indent))
            return 0;

        return sb_append(sb, "end");
    }

    if (!sb_append(sb, name))
        return 0;

    if (!sb_append(sb, " is "))
        return 0;

    return serialize_value(sb, value, 0);
}

int sdd_dumps(
    const Value* data,
    char** output,
    SDDError* error
) {
    if (!data || !output) {
        serializer_error(
            error,
            SDD_TYPE_ERROR,
            "Invalid serialization arguments"
        );

        return 0;
    }

    *output = NULL;

    if (data->type != VALUE_OBJECT) {
        serializer_error(
            error,
            SDD_TYPE_ERROR,
            "SDD root data must be an object"
        );

        return 0;
    }

    StringBuilder sb;

    if (!sb_init(&sb)) {
        serializer_error(
            error,
            SDD_MEMORY_ERROR,
            "Memory allocation failed"
        );

        return 0;
    }

    if (!sb_append(&sb, "SDD 1.0\n")) {
        free(sb.data);

        serializer_error(
            error,
            SDD_MEMORY_ERROR,
            "Memory allocation failed"
        );

        return 0;
    }

    const Object* object = &data->data.object;

    for (size_t i = 0; i < object->count; i++) {
        const ObjectEntry* entry = &object->entries[i];

        if (!entry->key || !entry->value) {
            free(sb.data);

            serializer_error(
                error,
                SDD_TYPE_ERROR,
                "Invalid object entry"
            );

            return 0;
        }

        if (!serialize_assignment(
            &sb,
            entry->key,
            entry->value,
            0
        ) || !sb_append_char(&sb, '\n')) {
            free(sb.data);

            serializer_error(
                error,
                SDD_MEMORY_ERROR,
                "Serialization failed"
            );

            return 0;
        }
    }

    *output = sb.data;
    return 1;
}

int sdd_dump(
    const Value* data,
    const char* filepath,
    SDDError* error
) {
    if (!filepath) {
        serializer_error(
            error,
            SDD_FILE_ERROR,
            "Invalid file path"
        );

        return 0;
    }

    if (!has_extension(filepath, ".sdd")) {
        serializer_error(
            error,
            SDD_FILE_ERROR,
            "Incorrect file extension. Use .sdd"
        );

        return 0;
    }

    char* source = NULL;

    if (!sdd_dumps(data, &source, error))
        return 0;

    FILE* file = fopen(filepath, "w");

    if (!file) {
        free(source);

        serializer_error(
            error,
            SDD_FILE_ERROR,
            "Could not open output file"
        );

        return 0;
    }

    size_t length = strlen(source);

    int success = fwrite(
        source,
        1,
        length,
        file
    ) == length;

    if (fclose(file) != 0)
        success = 0;

    free(source);

    if (!success) {
        serializer_error(
            error,
            SDD_FILE_ERROR,
            "Could not write SDD file"
        );

        return 0;
    }

    return 1;
}
