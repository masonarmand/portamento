#define STB_DXT_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "portamento.h"
#include "stb_dxt.h"

void fill_block(unsigned char* block, const unsigned char* data, int by, int bx, int width, int height, int channels);


void image_resize_and_compress(Image* image, int target_w, int target_h)
{
        unsigned char* resized_data = NULL;
        unsigned char* converted_data = NULL;
        unsigned char* compressed_data = NULL;
        unsigned char block[4 * 4 * 4];

        unsigned int i;
        int blocks_x;
        int blocks_y;
        int compressed_size;
        int by;
        int bx;

        static bytes_alloced = 0;
        static num_images = 0;

        resized_data = malloc(target_w * target_h * image->channels);
        if (!resized_data) {
                fprintf(stderr, "Failed to allocate memory for resized image\n");
                stbi_image_free(image->data);
                return;
        }

        if (!stbir_resize_uint8(image->data, image->width, image->height, 0, resized_data, target_w, target_h, 0, image->channels)) {
                fprintf(stderr, "Failed to resize image\n");
                stbi_image_free(image->data);
                return;
        }

        /* convert images with one channel into 3 channels */
        if (image->channels == 1) {
                converted_data = malloc(target_w * target_h * 3);
                if (!converted_data) {
                        fprintf(stderr, "Failed to allocate memory for converted image\n");
                        free(resized_data);
                        stbi_image_free(image->data);
                        return;
                }
                for (i = 0; i < target_w * target_h; i++) {
                        converted_data[i * 3 + 0] = resized_data[i];
                        converted_data[i * 3 + 1] = resized_data[i];
                        converted_data[i * 3 + 2] = resized_data[i];
                }
                image->channels = 3;
                free(resized_data);
                resized_data = converted_data;
        }

        blocks_x = (target_w + 3) / 4;
        blocks_y = (target_h + 3) / 4;
        compressed_size = blocks_x * blocks_y * 8;

        compressed_data = malloc(compressed_size);
        if (!compressed_data) {
                fprintf(stderr, "Failed to allocate memory for comrpessed image\n");
                free(resized_data);
                stbi_image_free(image->data);
                return;
        }

        for (by = 0; by < blocks_y; ++by) {
                for (bx = 0; bx < blocks_x; ++bx) {
                        fill_block(block, resized_data, by, bx, target_w, target_h, image->channels);
                        stb_compress_dxt_block(&compressed_data[(by * blocks_x + bx) * 8], block, 0, STB_DXT_NORMAL);
                }
        }

        image->width = target_w;
        image->height = target_h;
        image->num_bytes = compressed_size;
        free(resized_data);
        stbi_image_free(image->data);
        image->data = compressed_data;

        bytes_alloced += compressed_size;
        num_images += 1;
        printf(
                "total memory usage from images (GPU memory): %f MB (%d bytes). Image count = %d \n",
                (float) bytes_alloced / 1000000.0f,
                bytes_alloced,
                num_images
        );
}


void fill_block(unsigned char* block, const unsigned char* data, int by, int bx, int width, int height, int channels)
{
        unsigned int block_row;
        unsigned int block_col;
        for (block_row = 0; block_row < 4; ++block_row) {
                for (block_col = 0; block_col < 4; ++block_col) {
                        int src_x = bx * 4 + block_col;
                        int src_y = by * 4 + block_row;
                        int src_index = (src_y * width + src_x) * channels;
                        int dst_index = (block_row * 4 + block_col) * 4;

                        if (src_x > width && src_y > height) {
                                memset(&block[dst_index], 0, 4);
                                continue;
                        }
                        if (channels != 3) {
                                memcpy(&block[dst_index], &data[src_index], channels);
                                continue;
                        }

                        block[dst_index + 0] = data[src_index + 0];
                        block[dst_index + 1] = data[src_index + 1];
                        block[dst_index + 2] = data[src_index + 2];
                        block[dst_index + 3] = 255;
                }
        }
}


void gl_send_image(Image* image)
{
        GLuint tex;
        GLenum format;
        int x = image->width;
        int y = image->height;
        int n = image->channels;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        format = (n == 4) ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;

        glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, x, y, 0, image->num_bytes, image->data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image->data);
        image->data = NULL;

        image->nk_img = nk_image_id((int)tex);

        pthread_mutex_lock(&image->mutex);
        image->loaded = 1;
        pthread_cond_signal(&image->cond);
        pthread_mutex_unlock(&image->mutex);
}


void gl_send_uncompressed_image(Image* image)
{
        GLuint tex;
        GLenum format, internalFormat;
        int x = image->width;
        int y = image->height;
        int n = image->channels;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (n == 4) {
                format = GL_RGBA;
                internalFormat = GL_RGBA;
        }
        else if (n == 3) {
                format = GL_RGB;
                internalFormat = GL_RGB;
        }
        else if (n == 1) {
                format = GL_RED;
                internalFormat = GL_RED;
        }
        else {
                format = GL_RGBA;
                internalFormat = GL_RGBA;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, x, y, 0, format, GL_UNSIGNED_BYTE, image->data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image->data);
        image->data = NULL;

        image->nk_img = nk_image_id((int)tex);

        pthread_mutex_lock(&image->mutex);
        image->loaded = 1;
        pthread_cond_signal(&image->cond);
        pthread_mutex_unlock(&image->mutex);
}
