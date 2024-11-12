#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include <stdbool.h>

typedef struct {
        char* title;
        char* artist_name;
        char* album_title;
        char* album_artist_name;
        char* album_path;
        unsigned int track_number;
        unsigned int disc_number;
        unsigned int year;
        bool has_meta_cover;
} AudioMetadata;

unsigned audio_calculate_duration(const char* filepath);
void audio_get_metadata(const char* filepath, AudioMetadata* meta);
int audio_get_meta_image(const char* filepath, unsigned char** data, int* width, int* height, int* channels);
void audio_free_metadata(AudioMetadata* meta);
bool audio_is_valid_filetype(const char* filepath);

#endif
