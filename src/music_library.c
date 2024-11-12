#include "portamento.h"

int add_track_to_library(Track* track, MusicLibrary* library);


void handle_file(const char* path, MusicLibrary* library)
{
        Track* track;
        AudioMetadata* meta;

        if (!audio_is_valid_filetype(path))
                return;

        track = calloc(1, sizeof(Track));

        audio_get_metadata(path, &track->meta);
        meta = &track->meta;
        track->filename = strdup(path);
        track->duration = audio_calculate_duration(path);

        if (!meta->album_title || !meta->title || !meta->artist_name) {
                fprintf(stderr, "Missing metadata for %s\n", track->filename);
                free_track(track);
                return;
        }

        if (!meta->album_artist_name)
                meta->album_artist_name = strdup("");


        add_track_to_library(track, library);
}


int add_track_to_library(Track* track, MusicLibrary* library)
{
        Artist* artist;
        Artist* album_artist;
        Album* album;
        AudioMetadata* meta = &track->meta;
        bool is_new;

        artist = find_or_create_artist(library, meta->artist_name, &is_new);
        track->artist = artist;

        album = find_or_create_album(library, meta->album_title, meta->album_artist_name, meta->album_path, &is_new);
        if (is_new) {
                bool is_album_artist_new = false;
                album->year = meta->year;
                album->art_path = strdup("");
                album->art.loaded = false;

                /* if artist and album artist are different, AND the album artist is not blank */
                if ((strcmp(meta->album_artist_name, meta->artist_name) != 0)
                && (strcmp(meta->album_artist_name, "") != 0))
                        album_artist = find_or_create_artist(library, meta->album_artist_name, &is_album_artist_new);
                else if ((strcmp(meta->album_artist_name, "") == 0)) {
                        album_artist = track->artist;
                }

                append_album_node(&artist->albums_head, &artist->albums_tail, album, &artist->album_count);

                if (is_album_artist_new)
                        append_album_node(&album_artist->albums_head, &album_artist->albums_tail, album, &album_artist->album_count);

                album->album_artist = album_artist;

                if (!album->art.data) {
                        enqueue_album_cover(&library->album_cover_queue, library, track, album);
                }
        }
        else if ((strcmp(meta->album_artist_name, meta->artist_name) != 0) && album->album_artist != track->artist) {
                        bool is_album_artist_new = false;
                        Artist* album_artist = find_or_create_artist(library, "Various Artists", &is_album_artist_new);
                        if (is_album_artist_new)
                                append_album_node(&album_artist->albums_head, &album_artist->albums_tail, album, &album_artist->album_count);
                        album->album_artist = album_artist;

        }

        track->album = album;
        if (!artist_has_album(track->artist, album))
                append_album_node(&track->artist->albums_head, &track->artist->albums_tail, album, &track->artist->album_count);

        append_track_node(&album->tracks_head, &album->tracks_tail, track, &album->track_count);
        append_track(library, track);

        return 0;

}
