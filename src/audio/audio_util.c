#include "audio_util.h"
#include <string.h>
#include <stdlib.h>

char* get_directory_from_filepath(const char* filepath)
{
        char* dir = strdup(filepath);
        char* last_slash = strrchr(dir, '/');

        if (last_slash) {
                *last_slash = '\0';
        }
        else {
                free(dir);
                return NULL;
        }

        return dir;
}

