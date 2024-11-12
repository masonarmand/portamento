#include "portamento.h"

void init_album_path_set(AlbumPathSet* set);
int album_path_set_contains(AlbumPathSet* set, const char* album_path);
void album_path_set_add(AlbumPathSet* set, const char* album_path);


void init_album_cover_queue(AlbumCoverQueue* queue, size_t hash_set_bucket_count)
{
        queue->head = NULL;
        queue->tail = NULL;
        queue->finished = false;
        pthread_mutex_init(&queue->mutex, NULL);
        pthread_cond_init(&queue->cond, NULL);
        init_album_path_set(&queue->album_paths_set);
}


void free_album_cover_request(AlbumCoverRequest* req)
{
        free(req->album_path);
        free(req->music_filename);
        free(req);
}


void free_album_path_set(AlbumPathSet* set)
{
        AlbumPathEntry* current_entry;
        AlbumPathEntry* tmp;

        HASH_ITER(hh, set->entries, current_entry, tmp) {
                HASH_DEL(set->entries, current_entry);
                free(current_entry->album_path);
                free(current_entry);
        }
}


void free_album_cover_queue(AlbumCoverQueue* queue)
{
        AlbumCoverRequest* current;
        AlbumCoverRequest* next;

        pthread_mutex_lock(&queue->mutex);

        current = queue->head;
        while (current) {
                next = current->next;
                free_album_cover_request(current);
                current = next;
        }

        queue->head = NULL;
        queue->tail = NULL;
        free_album_path_set(&queue->album_paths_set);

        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
}


void enqueue_album_cover(AlbumCoverQueue* queue, MusicLibrary* library, Track* track, Album* album)
{
        pthread_mutex_lock(&queue->mutex);
        if (!album_path_set_contains(&queue->album_paths_set, album->album_path)) {
                AlbumCoverRequest* req = malloc(sizeof(AlbumCoverRequest));

                album_path_set_add(&queue->album_paths_set, album->album_path);

                req->album_path = strdup(album->album_path);
                req->music_filename = strdup(track->filename);
                req->has_meta_image = track->meta.has_meta_cover;
                req->library = library;
                req->album = album;
                req->next = NULL;

                if (!queue->tail) {
                        queue->head = req;
                        queue->tail = req;
                }
                else {
                        queue->tail->next = req;
                        queue->tail = req;
                }
                pthread_cond_signal(&queue->cond);
        }
        pthread_mutex_unlock(&queue->mutex);
}


void* music_library_thread(void* arg)
{
        MusicLibrary* library = (MusicLibrary*) arg;
        sqlite3* db;


        if (sqlite3_open("/home/mason/.portamento/db/music.db", &db) != SQLITE_OK) {
                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                return NULL;
        }

        db_create_schema(db);
        db_load_music_library(db, library);
        scan_new_music("/home/mason/.portamento/testdir/", db, library);
        album_sort(&library->album_list);
        db_save_music_library(db, library);
        sqlite3_close(db);

        pthread_mutex_lock(&library->mutex);
        library->scanning_done = true;
        pthread_mutex_unlock(&library->mutex);

        return NULL;
}


void* album_cover_loader_thread(void* arg)
{
        AlbumCoverQueue* queue = (AlbumCoverQueue*) arg;

        while (1) {
                AlbumCoverRequest* req;

                pthread_mutex_lock(&queue->mutex);
                while (!queue->head && !queue->finished)
                        pthread_cond_wait(&queue->cond, &queue->mutex);

                if (!queue->head && queue->finished) {
                        pthread_mutex_unlock(&queue->mutex);
                        break;
                }

                req = queue->head;
                queue->head = req->next;
                if (!queue->head)
                        queue->tail = NULL;
                pthread_mutex_unlock(&queue->mutex);

                req->album->art.loaded = false;

                load_thumbnail_image(&req->album->art, req->album_path, req->music_filename, req->album->title, req->has_meta_image);
                /*image_resize_and_compress(&req->album->art, 154, 154);*/
                image_resize_and_compress(&req->album->art, 300, 300);
                emit_album_art_loaded_event(req->library, req->album);

                pthread_mutex_init(&req->album->art.mutex, NULL);
                pthread_cond_init(&req->album->art.cond, NULL);

                pthread_mutex_lock(&req->album->art.mutex);
                while(!req->album->art.loaded)
                        pthread_cond_wait(&req->album->art.cond, &req->album->art.mutex);
                pthread_mutex_unlock(&req->album->art.mutex);

                printf("album cover passed to opengl\n");

                free(req->album_path);
                free(req->music_filename);
                free(req);

        }

        return NULL;
}


void init_album_path_set(AlbumPathSet* set)
{
        set->entries = NULL;
}

int album_path_set_contains(AlbumPathSet* set, const char* album_path)
{
        AlbumPathEntry* entry;
        HASH_FIND_STR(set->entries, album_path, entry);
        return (entry != NULL);
}

void album_path_set_add(AlbumPathSet* set, const char* album_path)
{
        AlbumPathEntry* entry = malloc(sizeof(AlbumPathEntry));
        entry->album_path = strdup(album_path);
        HASH_ADD_KEYPTR(hh, set->entries, entry->album_path, strlen(entry->album_path), entry);
}

