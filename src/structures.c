#include "portamento.h"

int compare_artists(Artist* a, Artist* b);
int compare_albums(const Album* a, const Album* b, bool compare_artists);
int compare_tracks(Track* a, Track* b, bool compare_artists);

char* str_to_lower(const char* str)
{
        char* lower_str = strdup(str);
        char* p;

        if (!lower_str)
                return NULL;

        for (p = lower_str; *p; ++p)
                *p = tolower((unsigned char)*p);

        return lower_str;
}


void append_track_node(TrackNode** head, TrackNode** tail, Track* track, int* count)
{
        TrackNode* new_node = malloc(sizeof(TrackNode));
        new_node->track = track;
        new_node->next = NULL;
        new_node->prev = NULL;

        if (!*head) {
                *head = new_node;
                *tail = new_node;
        }
        else {
                TrackNode* current = *head;
                TrackNode* previous = NULL;

                while (current && compare_tracks(current->track, track, false) <= 0) {
                        previous = current;
                        current = current->next;
                }

                if (!previous) {
                        new_node->next = *head;
                        (*head)->prev = new_node;
                        *head = new_node;
                }
                else if (!current) {
                        previous->next = new_node;
                        new_node->prev = previous;
                        *tail = new_node;
                }
                else {
                        previous->next = new_node;
                        new_node->prev = previous;
                        new_node->next = current;
                        current->prev = new_node;
                }
        }
        (*count)++;
}


void free_track_node_list(TrackNode* head)
{
        TrackNode* current = head;
        TrackNode* next;

        while (current) {
                next = current->next;
                free(current);
                current = next;
        }
}


void append_album_node(AlbumNode** head, AlbumNode** tail, Album* album, int* count)
{
        AlbumNode* new_node = malloc(sizeof(AlbumNode));
        new_node->album = album;
        new_node->next = NULL;
        new_node->prev = NULL;

        if (!*head) {
                *head = new_node;
                *tail = new_node;
        }
        else {
                AlbumNode* current = *head;
                AlbumNode* previous = NULL;

                while (current && compare_albums(current->album, album, false) <= 0) {
                        previous = current;
                        current = current->next;
                }

                if (!previous) {
                        new_node->next = *head;
                        (*head)->prev = new_node;
                        *head = new_node;
                }
                else if (!current) {
                        previous->next = new_node;
                        new_node->prev = previous;
                        *tail = new_node;
                }
                else {
                        previous->next = new_node;
                        new_node->prev = previous;
                        new_node->next = current;
                        current->prev = new_node;
                }
        }
        (*count)++;
}


void free_album_node_list(AlbumNode* head)
{
        AlbumNode* current = head;
        AlbumNode* next;

        while (current) {
                next = current->next;
                free(current);
                current = next;
        }
}


void append_track(MusicLibrary* library, Track* new_track)
{
        TrackList* list = &library->track_list;

        if (!list->head) {
                list->head = new_track;
                list->tail = new_track;
        }
        else {
                Track* current = list->head;
                Track* previous = NULL;

                while (current && compare_tracks(current, new_track, true) <= 0) {
                        previous = current;
                        current = current->next;
                }

                if (!previous) {
                        new_track->next = list->head;
                        list->head->prev = new_track;
                        list->head = new_track;
                }
                else if (!current) {
                        previous->next = new_track;
                        new_track->prev = previous;
                        list->tail = new_track;
                }
                else {
                        previous->next = new_track;
                        new_track->prev = previous;
                        new_track->next = current;
                        current->prev = new_track;
                }
        }

        list->count++;
        emit_track_loaded_event(library, new_track);
}


void free_track(Track* track)
{
        audio_free_metadata(&track->meta);
        free(track->filename);
        free(track);
}


void free_track_list(TrackList* list)
{
        Track* current = list->head;
        Track* next;

        while (current) {
                next = current->next;
                free_track(current);
                current = next;
        }

        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
}


void append_album(MusicLibrary* library, Album* new_album) {
        AlbumList* list = &library->album_list;

        if (!list->head) {
                list->head = new_album;
                list->tail = new_album;
        }
        else {
                list->tail->next = new_album;
                new_album->prev = list->tail;
                list->tail = new_album;
        }

        list->count++;
        emit_album_loaded_event(library, new_album);
}


void free_album_list(AlbumList* list)
{
        Album* current = list->head;
        Album* next;

        while (current) {
                next = current->next;
                if (current->art.data)
                        free(current->art.data);
                free(current->title);
                free(current->album_path);
                free(current->art_path);
                free(current->artist_name);
                free_track_node_list(current->tracks_head);
                free(current);
                current = next;
        }

        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
}


void append_artist(MusicLibrary* library, Artist* new_artist)
{
        ArtistList* list = &library->artist_list;
        if (!list->head) {
                list->head = new_artist;
                list->tail = new_artist;
        }
        else {
                Artist* current = list->head;
                Artist* previous = NULL;

                while (current && compare_artists(current, new_artist) <= 0) {
                        previous = current;
                        current = current->next;
                }

                if (!previous) {
                        new_artist->next = list->head;
                        list->head->prev = new_artist;
                        list->head = new_artist;
                }
                else if (!current) {
                        previous->next = new_artist;
                        new_artist->prev = previous;
                        list->tail = new_artist;
                }
                else {
                        previous->next = new_artist;
                        new_artist->prev = previous;
                        new_artist->next = current;
                        current->prev = new_artist;
                }
        }

        list->count++;
        emit_artist_loaded_event(library, new_artist);
}


void free_artist_list(ArtistList* list)
{
        Artist* current = list->head;
        Artist* next;

        while (current) {
                next = current->next;
                free(current->name);
                free_album_node_list(current->albums_head);
                free(current);
                current = next;
        }

        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
}


void free_music_library(MusicLibrary* library)
{
        free_track_list(&library->track_list);
        free_album_list(&library->album_list);
        free_artist_list(&library->artist_list);
        free_event_queue(&library->event_queue);
        free_album_cover_queue(&library->album_cover_queue);
        pthread_mutex_destroy(&library->mutex);
        pthread_cond_destroy(&library->cond);
}


int compare_artists(Artist* a, Artist* b)
{
        char* a_name = str_to_lower(a->name);
        char* b_name = str_to_lower(b->name);
        int cmp = strcmp(a_name, b_name);
        free(a_name);
        free(b_name);
        return cmp;
}


int compare_tracks(Track* a, Track* b, bool compare_artists)
{
        int album_cmp;

        if (compare_artists) {
                const char* a_name = get_album_artist_name(a->album);
                const char* b_name = get_album_artist_name(b->album);
                int artist_cmp;

                if (!a_name && b_name)
                        return 1;
                if (a_name && !b_name)
                        return -1;

                if (a_name && b_name) {
                        char* a_lower_name = str_to_lower(a_name);
                        char* b_lower_name = str_to_lower(b_name);
                        artist_cmp = strcmp(a_lower_name, b_lower_name);
                        free(a_lower_name);
                        free(b_lower_name);
                        if (artist_cmp != 0)
                                return artist_cmp;
                }
        }

        if (a->meta.year != b->meta.year)
                return a->meta.year - b->meta.year;

        album_cmp = strcmp(a->meta.album_title, b->meta.album_title);
        if (album_cmp != 0)
                return album_cmp;

        if (a->meta.disc_number != b->meta.disc_number)
                return a->meta.disc_number - b->meta.disc_number;

        if (a->meta.track_number != b->meta.track_number)
                return a->meta.track_number - b->meta.track_number;

        return 0;
}


int compare_albums(const Album* a, const Album* b, bool compare_artists)
{
        if (compare_artists) {
                const char* a_name = get_album_artist_name(a);
                const char* b_name = get_album_artist_name(b);
                int artist_cmp;

                if (!a_name && b_name)
                        return 1;
                if (a_name && !b_name)
                        return -1;

                if (a_name && b_name) {
                        char* a_lower_name = str_to_lower(a_name);
                        char* b_lower_name = str_to_lower(b_name);
                        artist_cmp = strcmp(a_lower_name, b_lower_name);
                        free(a_lower_name);
                        free(b_lower_name);
                        if (artist_cmp != 0)
                                return artist_cmp;
                }
        }

        if (a->year != b->year)
                return a->year - b->year;

        return strcmp(a->title, b->title);
}


AlbumNode* find_or_create_album_node(Artist* artist, Album* album)
{
        AlbumNode* current = artist->albums_head;
        while (current) {
                if (current->album == album)
                        return current;
                current = current->next;
        }
        append_album_node(&artist->albums_head, &artist->albums_tail, album, &artist->album_count);
        return artist->albums_tail;
}


Artist* find_artist(ArtistList* artist_list, const char* name)
{
        Artist* current = artist_list->head;
        while (current) {
                if (strcmp(current->name, name) == 0)
                        return current;
                current = current->next;
        }
        return NULL;
}


Artist* find_or_create_artist(MusicLibrary* library, const char* name, bool* is_new)
{
        Artist* new_artist;
        ArtistList* artist_list = &library->artist_list;

        if (strcmp(name, "") == 0)
                return NULL;

        new_artist = find_artist(artist_list, name);
        *is_new = (!new_artist);


        if (*is_new) {
                new_artist = calloc(1, sizeof(Artist));
                new_artist->name = strdup(name);
                new_artist->album_count = 0;
                append_artist(library, new_artist);
        }

        return new_artist;

}


Album* find_album(AlbumList* album_list, const char* title, const char* artist_name, const char* album_path)
{
        Album* current = album_list->head;
        while (current) {
                if (strcmp(current->title, title) == 0
                && strcmp(current->artist_name, artist_name) == 0
                && strcmp(current->album_path, album_path) == 0)
                        return current;
                current = current->next;
        }
        return NULL;
}


Album* find_or_create_album(MusicLibrary* library, const char* title, const char* artist_name, const char* album_path, bool* is_new)
{
        Album* new_album;
        AlbumList* album_list = &library->album_list;

        new_album = find_album(album_list, title, artist_name, album_path);
        *is_new = (!new_album);

        if (*is_new) {
                new_album = calloc(1, sizeof(Album));
                new_album->title = strdup(title);
                new_album->artist_name = strdup(artist_name);
                new_album->album_path = strdup(album_path);
                new_album->tracks_head = NULL;
                new_album->tracks_tail = NULL;
                new_album->track_count = 0;

                append_album(library, new_album);
        }

        return new_album;
}


bool artist_has_album(Artist* artist, Album* album)
{
        AlbumNode* current = artist->albums_head;
        while (current) {
                if (current->album == album)
                        return true;
                current = current->next;
        }
        return false;
}


const char* get_album_artist_name(const Album* album)
{
        if (strcmp(album->artist_name, "") != 0)
                return album->artist_name;
        else if (album->album_artist)
                return album->album_artist->name;
        else if (album->tracks_head)
                return album->tracks_head->track->meta.artist_name;
        else
                return NULL;
}


void album_sort(AlbumList* list)
{
        Album* sorted = NULL;
        Album* current = list->head;

        if (!list->head || !list->head->next)
                return;

        while (current) {
                Album* next = current->next;
                current->prev = current->next = NULL;

                if (!sorted) {
                        sorted = current;
                }
                else if (compare_albums(current, sorted, true) <= 0) {
                        current->next = sorted;
                        sorted->prev = current;
                        sorted = current;
                }
                else {
                        Album* sorted_current = sorted;
                        while (sorted_current->next && compare_albums(current, sorted_current->next, true) > 0) {
                                sorted_current = sorted_current->next;
                        }
                        current->next = sorted_current->next;
                        if (sorted_current->next) {
                                sorted_current->next->prev = current;
                        }
                        sorted_current->next = current;
                        current->prev = sorted_current;
                }

                current = next;
        }

        list->head = sorted;
        Album* sorted_tail = sorted;
        while (sorted_tail->next) {
                sorted_tail = sorted_tail->next;
        }
        list->tail = sorted_tail;
}


int get_artist_track_count(Artist* artist)
{
        AlbumNode* current = artist->albums_head;
        int count = 0;

        while (current) {
                count += current->album->track_count;
                current = current->next;
        }

        return count;
}


void init_music_library(MusicLibrary* library)
{
        library->track_list.head = NULL;
        library->track_list.tail = NULL;
        library->track_list.count = 0;

        library->album_list.head = NULL;
        library->album_list.tail = NULL;
        library->album_list.count = 0;

        library->artist_list.head = NULL;
        library->artist_list.tail = NULL;
        library->artist_list.count = 0;
        library->scanning_done = false;

        init_event_queue(&library->event_queue);
        init_album_cover_queue(&library->album_cover_queue, 64);

        pthread_mutex_init(&library->mutex, NULL);
        pthread_cond_init(&library->cond, NULL);
}
