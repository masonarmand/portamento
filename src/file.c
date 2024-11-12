#include "portamento.h"

void get_files_in_directory(const char* directory, char*** files, int* count);
int is_dir(const char* path);

int load_image_from_file(const char* filepath, Image* image)
{
        int width;
        int height;
        int channels;
        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);

        if (!data) {
                fprintf(stderr, "Failed to load image from %s\n", filepath);
                return -1;
        }

        image->data = data;
        image->width = width;
        image->height = height;
        image->channels = channels;
        return 0;
}


void load_thumbnail_image(Image* image, const char* album_path, char* music_filename, const char* album_title, bool has_meta_image)
{
        /*Image* thumb = calloc(1, sizeof(Image));*/
        /*image->data = malloc(ALBUM_THUMBNAIL_SIZE * ALBUM_THUMBNAIL_SIZE * 3 * sizeof(image->data));*/
        /*printf("Loading %s\n", album_title);*/
        /*if (has_meta_image) {
                audio_get_meta_image(music_filename, &image->data, &image->width, &image->height, &image->channels);
        }
        else*/
        if (load_dir_image(album_path, album_title, image) != 0) {
                fprintf(stderr, "Failed to load cover image for %s", album_title);
                return;
        }
        /*
        if (image->data)
                image->nk_img = render_image_load(&image->data, image->width, image->height, image->channels);
        */

        /*if (thumb->data) {
                stbir_resize_uint8(thumb->data, thumb->width, thumb->height, 0, image->data, ALBUM_THUMBNAIL_SIZE, ALBUM_THUMBNAIL_SIZE, 0, 3);
                image->width = ALBUM_THUMBNAIL_SIZE;
                image->height = ALBUM_THUMBNAIL_SIZE;
                image->channels = 3;
                free(thumb);
        }*/
}


const char* get_file_extension(const char* filename)
{
        const char* dot = strrchr(filename, '.');
        if (!dot || dot == filename)
                return "";
        return dot + 1;
}


int load_dir_image(const char* directory, const char* album_title, Image* image)
{
        const char* extensions[] = {".jpg", ".jpeg", ".png", ".bmp", ".gif", NULL};
        const char* filenames[] = {"cover", "album"};
        char filepath[PATH_MAX];
        unsigned int i;

        for (i = 0; extensions[i]; i++) {
                unsigned int j;
                for (j = 0; j < 2; j++) {
                        snprintf(filepath, sizeof(filepath), "%s/%s%s", directory, filenames[j], extensions[i]);
                        if (access(filepath, F_OK) != -1)
                                return load_image_from_file(filepath, image);
                }
                snprintf(filepath, sizeof(filepath), "%s/%s%s", directory, album_title, extensions[i]);
                if (access(filepath, F_OK) != -1)
                        return load_image_from_file(filepath, image);
        }

        return -1;
}


void scan_new_music(const char* dir, sqlite3* db, MusicLibrary* library)
{
        char** files;
        int file_count = 0;
        unsigned int i;

        get_files_in_directory(dir, &files, &file_count);

        for (i = 0; i < file_count; i++) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s%s", dir, files[i]);

                if (is_dir(full_path)) {
                        char dir_path[1024];
                        snprintf(dir_path, sizeof(dir_path), "%s/", full_path);
                        scan_new_music(dir_path, db, library);
                }
                else if (!db_file_in_database(db, full_path)) {
                        handle_file(full_path, library);
                }
                free(files[i]);
        }

        free(files);
}


void get_files_in_directory(const char* directory, char*** files, int* count)
{
        DIR* dir;
        struct dirent* entry;

        if ((dir = opendir(directory)) == NULL) {
                perror("opendir");
                return;
        }

        *count = 0;
        *files = NULL;

        while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                }
                *files = realloc(*files, (*count + 1) * sizeof(char*));
                (*files)[*count] = strdup(entry->d_name);
                (*count)++;
        }

        closedir(dir);
}


int is_dir(const char* path)
{
        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
                return 0;
        }
        return S_ISDIR(statbuf.st_mode);
}
