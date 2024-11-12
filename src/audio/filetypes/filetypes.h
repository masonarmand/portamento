#ifndef FILETYPES_H
#define FILETYPES_H

#include <FLAC/all.h>
#include <string.h>
#include "../audio_handler.h"
#include "../audio_util.h"

typedef enum {
        FILE_TYPE_FLAC,
        FILE_TYPE_UNKNOWN
} FileType;

typedef struct {
        unsigned (*calculate_duration)(const char* filepath);
        void (*get_metadata)(const char* filepath, AudioMetadata* meta);
        int (*get_meta_image)(const char* filepath, unsigned char** data, int* width, int* height, int* channels);
} FileTypeHandler;

/* flac handler functions */
unsigned calculate_flac_duration(const char* filepath);

/* FileTypeHandlers */
extern FileTypeHandler flac_handler;

#endif
