/*
**  File: SDD/CSDD/sdd.h
**
**  Creator: Aleksander Sapieha (sourceguy68@gmail.com)
**
**  Contributors:
**  Aleksander Sapieha (sourceguy68@gmail.com)
*/

#ifndef _SDD_H
#define _SDD_H


#include "sdd_parser.h"

static int has_extension(
    const char* filepath,
    const char* extension
);

static void set_error(
    SDDError* error,
    SDDErrorType type,
    const char* message
);

int sdd_loads(
    const char* source,
    SDDDocument* document,
    SDDError* error
);

int sdd_load(
    const char* filepath,
    SDDDocument* document,
    SDDError* error
);

int sdd_dumps(
    const Value* data,
    char** output,
    SDDError* error
);

int sdd_dump(
    const Value* data,
    const char* filepath,
    SDDError* error
);

#endif // _SDD_H
