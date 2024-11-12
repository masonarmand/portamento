#include "filetypes.h"

unsigned calculate_flac_duration(const char* filepath);
void get_flac_metadata(const char* filepath, AudioMetadata* meta);
int get_flac_meta_image(const char* filepath, unsigned char** data, int* width, int* height, int* channels);

FileTypeHandler flac_handler = {
    .calculate_duration = calculate_flac_duration,
    .get_metadata = get_flac_metadata,
    .get_meta_image = get_flac_meta_image,
};


typedef struct {
        unsigned total_samples;
        unsigned sample_rate;
} FlacInfo;


FLAC__StreamDecoderWriteStatus flac_write_callback(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data)
{
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flac_metadata_callback(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data)
{
        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
                FlacInfo* info = (FlacInfo*)client_data;
                info->total_samples = metadata->data.stream_info.total_samples;
                info->sample_rate = metadata->data.stream_info.sample_rate;
        }
}


void flac_error_callback(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data)
{
        return;
}


unsigned calculate_flac_duration(const char* filepath)
{
        FlacInfo info = {0, 0};
        FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
        FLAC__StreamDecoderInitStatus init_status;

        if (!decoder)
                return 0;

        init_status = FLAC__stream_decoder_init_file(decoder, filepath, flac_write_callback, flac_metadata_callback, flac_error_callback, &info);
        if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
                FLAC__stream_decoder_delete(decoder);
                return 0;
        }

        FLAC__stream_decoder_process_until_end_of_metadata(decoder);
        FLAC__stream_decoder_finish(decoder);
        FLAC__stream_decoder_delete(decoder);

        return (info.sample_rate > 0) ? (info.total_samples / info.sample_rate) : 0;
}


int init_flac_metadata_chain(const char* filepath, FLAC__Metadata_Chain** chain, FLAC__Metadata_Iterator** iterator)
{
        *chain = FLAC__metadata_chain_new();
        *iterator = FLAC__metadata_iterator_new();

        if (!*chain || !*iterator) {
                fprintf(stderr, "Failed to allocate memory for FLAC metadata chain or iterator\n");
                if (*chain)
                        FLAC__metadata_chain_delete(*chain);
                if (*iterator)
                        FLAC__metadata_iterator_delete(*iterator);
                return -1;
        }

        if (!FLAC__metadata_chain_read(*chain, filepath)) {
                fprintf(stderr, "Failed to read FLAC metadata from %s\n", filepath);
                FLAC__metadata_chain_delete(*chain);
                FLAC__metadata_iterator_delete(*iterator);
                return -1;
        }

        FLAC__metadata_iterator_init(*iterator, *chain);
        return 0;
}


void get_flac_metadata(const char* filepath, AudioMetadata* meta)
{
        FLAC__StreamMetadata* metadata = NULL;
        FLAC__Metadata_Chain* chain = NULL;
        FLAC__Metadata_Iterator* iterator = NULL;

        if (init_flac_metadata_chain(filepath, &chain, &iterator) != 0) {
                fprintf(stderr, "Failed to init flac metadata chain.\n");
                return;
        }

        /*meta = calloc(1, sizeof(AudioMetadata));*/
        meta->has_meta_cover = false;
        meta->album_path = get_directory_from_filepath(filepath);

        while (FLAC__metadata_iterator_next(iterator)) {
                metadata = FLAC__metadata_iterator_get_block(iterator);
                if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
                        FLAC__StreamMetadata_VorbisComment* vorbis_comment = &metadata->data.vorbis_comment;
                        unsigned int i;

                        for (i = 0; i < vorbis_comment->num_comments; ++i) {
                                FLAC__StreamMetadata_VorbisComment_Entry entry = vorbis_comment->comments[i];
                                char* field = strndup((char*)entry.entry, entry.length);

                                if (strncmp(field, "TITLE=", 6) == 0) {
                                        meta->title = strdup(field + 6);
                                }
                                else if (strncmp(field, "ARTIST=", 7) == 0) {
                                        meta->artist_name = strdup(field + 7);
                                }
                                else if (strncmp(field, "ALBUM=", 6) == 0) {
                                        meta->album_title = strdup(field + 6);
                                }
                                else if (strncmp(field, "ALBUMARTIST=", 12) == 0) {
                                        meta->album_artist_name = strdup(field + 12);
                                }
                                else if (strncmp(field, "DATE=", 5) == 0) {
                                        meta->year = atoi(field + 5);
                                }
                                else if (strncmp(field, "TRACKNUMBER=", 12) == 0) {
                                        meta->track_number = atoi(field + 12);
                                }
                                else if (strncmp(field, "DISCNUMBER=", 11) == 0) {
                                        meta->disc_number = atoi(field + 11);
                                }

                                free(field);
                        }
                }
                else if (metadata->type == FLAC__METADATA_TYPE_PICTURE) {
                        meta->has_meta_cover = true;
                }
        }

        FLAC__metadata_chain_delete(chain);
        FLAC__metadata_iterator_delete(iterator);
}


int get_flac_meta_image(const char* filepath, unsigned char** data, int* width, int* height, int* channels)
{
        FLAC__StreamMetadata* metadata = NULL;
        FLAC__Metadata_Chain* chain = NULL;
        FLAC__Metadata_Iterator* iterator = NULL;

        init_flac_metadata_chain(filepath, &chain, &iterator);

        while (FLAC__metadata_iterator_next(iterator)) {
                metadata = FLAC__metadata_iterator_get_block(iterator);

                if (metadata->type == FLAC__METADATA_TYPE_PICTURE) {
                         FLAC__StreamMetadata_Picture* picture = &metadata->data.picture;
                         *data = malloc(picture->data_length);

                         if (!*data) {
                                 fprintf(stderr, "Failed to allocate memory for image data\n");
                                 return -1;
                         }

                         memcpy(*data, picture->data, picture->data_length);
                         *width = picture->width;
                         *height = picture->height;
                         *channels = 3;
                         printf("Loaded image from metadata: %dx%d, %d channels, %u bytes\n", *width, *height, *channels, picture->data_length);
                         break;
                }
        }

        FLAC__metadata_chain_delete(chain);
        FLAC__metadata_iterator_delete(iterator);

        return 0;
}
