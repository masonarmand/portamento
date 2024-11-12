#define STB_IMAGE_IMPLEMENTATION

#include "portamento.h"

int main(void)
{
        MusicLibrary library;
        pthread_t album_art_thread;
        pthread_t library_thread;

        init_music_library(&library);

        pthread_create(&album_art_thread, NULL, album_cover_loader_thread, &library.album_cover_queue);
        pthread_create(&library_thread, NULL, music_library_thread, &library);

        draw_music_library(&library);

        pthread_kill(library_thread, NULL);
        pthread_kill(album_art_thread, NULL);

        free_music_library(&library);

        /*image.data = stbi_load("/home/mason/Desktop/shadow.png", &image.width, &image.height, &image.channels, 0);
        if (!image.data) {
                printf("error\n");
                return 0;
        }
        image_resize_and_compress(&image, 184, 184);
        for (p = 0; p < image.num_bytes; p++) {
                printf("%d,", image.data[p]);
        }
        printf("\n");*/


        return 0;
}
