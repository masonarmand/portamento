#ifndef PORTAMENTO_H
#define PORTAMENTO_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <sys/stat.h>
#include <FLAC/metadata.h>
#include <FLAC/stream_decoder.h>
#include "sqlite3.h"
#include "stb_image.h"
#include "stb_image_resize.h"
#include "nuklear.h"
#include "uthash.h"

#include "audio/audio_handler.h"

#define ALBUM_THUMBNAIL_SIZE 150
#define ALBUM_FULL_IMAGE_SIZE 445

typedef struct Track Track;
typedef struct Album Album;
typedef struct Artist Artist;
typedef struct MusicLibrary MusicLibrary;
typedef struct AlbumCoverRequest AlbumCoverRequest;
typedef struct Event Event;

typedef enum {
        EVENT_TRACK_LOADED,
        EVENT_ALBUM_LOADED,
        EVENT_ARTIST_LOADED,
        EVENT_ALBUM_ART_LOADED
} EventType;

struct Event {
        EventType type;
        union {
                struct Track* track;
                struct Album* album;
                struct Artist* artist;
        } data;
        Event* next;
};

typedef struct {
        Event* head;
        Event* tail;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
} EventQueue;

typedef struct {
        unsigned char* data;
        unsigned int width;
        unsigned int height;
        unsigned int channels;
        unsigned int num_bytes;
        struct nk_image nk_img;
        bool loaded;

        pthread_mutex_t mutex;
        pthread_cond_t cond;
} Image;

typedef struct TrackNode {
        Track* track;
        struct TrackNode* next;
        struct TrackNode* prev;
} TrackNode;

typedef struct AlbumNode {
        Album* album;
        struct AlbumNode* next;
        struct AlbumNode* prev;
} AlbumNode;

struct Track {
        char* filename;
        AudioMetadata meta;
        unsigned int duration;

        Album* album;
        Artist* artist;

        /* for the master track list */
        Track* next;
        Track* prev;
};

struct Album {
        char* title;
        unsigned int year;
        char* album_path;
        char* art_path;
        char* artist_name;
        TrackNode* tracks_head;
        TrackNode* tracks_tail;
        int track_count;
        Image art;
        Image art_full_size;
        Artist* album_artist;

        /* for the master album list */
        Album* next;
        Album* prev;
};

struct Artist {
        char* name;
        AlbumNode* albums_head;
        AlbumNode* albums_tail;
        int album_count;

        /* for the master artist list */
        Artist* next;
        Artist* prev;
};

typedef struct {
        Artist* head;
        Artist* tail;
        unsigned int count;
} ArtistList;

typedef struct {
        Album* head;
        Album* tail;
        unsigned int count;
} AlbumList;

typedef struct {
        Track* head;
        Track* tail;
        unsigned int count;
} TrackList;

typedef struct {
        char* album_path;
        UT_hash_handle hh;
} AlbumPathEntry;

typedef struct {
        AlbumPathEntry* entries;
} AlbumPathSet;

struct AlbumCoverRequest {
        char* album_path;
        char* music_filename;
        bool has_meta_image;
        Album* album;
        MusicLibrary* library;
        AlbumCoverRequest* next;
};

typedef struct {
        AlbumCoverRequest* head;
        AlbumCoverRequest* tail;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        AlbumPathSet album_paths_set;
        bool finished;
} AlbumCoverQueue;

struct MusicLibrary {
        TrackList track_list;
        AlbumList album_list;
        ArtistList artist_list;

        AlbumCoverQueue album_cover_queue;
        EventQueue event_queue;

        pthread_mutex_t mutex;
        pthread_cond_t cond;

        int scanning_done;
};

/* structures.c functions */
void append_track_node(TrackNode** head, TrackNode** tail, Track* track, int* count);
void free_track_node_list(TrackNode* head);
void append_album_node(AlbumNode** head, AlbumNode** tail, Album* album, int* count);
void free_album_node_list(AlbumNode* head);
void append_track(MusicLibrary* library, Track* new_track);
void free_track(Track* track);
void free_track_list(TrackList* list);
void append_album(MusicLibrary* library, Album* new_album);
void free_album_list(AlbumList* list);
void append_artist(MusicLibrary* library, Artist* new_artist);
void free_artist_list(ArtistList* list);
void free_music_library(MusicLibrary* library);
AlbumNode* find_or_create_album_node(Artist* artist, Album* album);
Artist* find_artist(ArtistList* artist_list, const char* name);
Artist* find_or_create_artist(MusicLibrary* library, const char* name, bool* is_new);
Album* find_album(AlbumList* album_list, const char* title, const char* artist_name, const char* album_path);
Album* find_or_create_album(MusicLibrary* library, const char* title, const char* artist_name, const char* album_path, bool* is_new);
int add_track_to_library(Track* track, MusicLibrary* library);
bool artist_has_album(Artist* artist, Album* album);
const char* get_album_artist_name(const Album* album);
void album_sort(AlbumList* list);
void init_music_library(MusicLibrary* library);
bool artist_has_album(Artist* artist, Album* album);
int get_artist_track_count(Artist* artist);

/* file.c functions */
int load_image_from_file(const char* filepath, Image* image);
void load_thumbnail_image(Image* image, const char* album_path, char* music_filename, const char* album_title, bool has_meta_image);
const char* get_file_extension(const char* filename);
int load_dir_image(const char* directory, const char* album_title, Image* image);
char* get_directory_from_filepath(const char* filepath);
void scan_new_music(const char* dir, sqlite3* db, MusicLibrary* library);

/* music_library.c functions */
void handle_file(const char* path, MusicLibrary* library);

/* database.c functions */
int db_load_music_library(sqlite3* db, MusicLibrary* library);
int db_save_music_library(sqlite3* db, MusicLibrary* library);
bool db_file_in_database(sqlite3* db, const char* filename);
int db_create_schema(sqlite3* db);

/* audio/flac.c functions */
unsigned calculate_flac_duration(const char* filepath);

/* event.c functions */
void init_event_queue(EventQueue* queue);
void free_event(Event* event);
void free_event_queue(EventQueue* queue);
Event* dequeue_event(EventQueue* queue);
Event* try_dequeue_event(EventQueue* queue);
void emit_track_loaded_event(MusicLibrary* library, Track* track);
void emit_album_loaded_event(MusicLibrary* library, Album* album);
void emit_artist_loaded_event(MusicLibrary* library, Artist* artist);
void emit_album_art_loaded_event(MusicLibrary* library, Album* album);

/* thread.c functions */
void enqueue_album_cover(AlbumCoverQueue* queue, MusicLibrary* library, Track* track, Album* album);
void init_album_cover_queue(AlbumCoverQueue* queue, size_t hash_set_bucket_count);
void free_album_cover_request(AlbumCoverRequest* req);
void free_album_cover_queue(AlbumCoverQueue* queue);
void* music_library_thread(void* arg);
void* album_cover_loader_thread(void* arg);

/* image.c */
void image_resize_and_compress(Image* image, int target_w, int target_h);
void gl_send_image(Image* image);
void gl_send_uncompressed_image(Image* image);

/* render.c */
void draw_music_library(MusicLibrary* library);

/* images */
extern unsigned char* img_shadow;

#endif
