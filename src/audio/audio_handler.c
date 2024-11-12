#include "audio_handler.h"
#include "filetypes/filetypes.h"
#include <string.h>

typedef struct {
        const char* extension;
        FileType type;
} FileTypeEntry;

FileTypeEntry file_types[] = {
        {".flac", FILE_TYPE_FLAC},
        {NULL, 0}
};


FileType detect_file_type(const char* filepath)
{
        const char* ext = strrchr(filepath, '.');
        unsigned int i;

        if (!ext)
                return FILE_TYPE_UNKNOWN;

        for (i = 0; file_types[i].extension != NULL; i++) {
                if (strcmp(file_types[i].extension, ext) == 0)
                        return file_types[i].type;
        }

        return FILE_TYPE_UNKNOWN;
}


static FileTypeHandler get_file_type_handler(FileType type)
{
        FileTypeHandler empty_handler = {0, 0};

        switch (type) {
        case FILE_TYPE_FLAC:
                return flac_handler;
        default:
                return empty_handler;
        }

        return empty_handler;
}


unsigned audio_calculate_duration(const char* filepath)
{
        FileType type = detect_file_type(filepath);
        FileTypeHandler handler = get_file_type_handler(type);

        if (handler.calculate_duration)
                return handler.calculate_duration(filepath);
        return 0;
}


void audio_get_metadata(const char* filepath, AudioMetadata* meta)
{
        FileType type = detect_file_type(filepath);
        FileTypeHandler handler = get_file_type_handler(type);

        if (handler.get_metadata)
                handler.get_metadata(filepath, meta);
}


int audio_get_meta_image(const char* filepath, unsigned char** data, int* width, int* height, int* channels)
{
        FileType type = detect_file_type(filepath);
        FileTypeHandler handler = get_file_type_handler(type);

        if (handler.get_meta_image)
                return handler.get_meta_image(filepath, data, width, height, channels);

        return -1;
}


bool audio_is_valid_filetype(const char* filepath)
{
        FileType type = detect_file_type(filepath);
        return (type != FILE_TYPE_UNKNOWN);
}


void audio_free_metadata(AudioMetadata* meta)
{
        free(meta->title);
        free(meta->artist_name);
        free(meta->album_title);
        free(meta->album_artist_name);
        free(meta->album_path);
}
