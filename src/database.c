#include "portamento.h"

int db_load_artists(sqlite3* db, MusicLibrary* library);
int db_load_albums(sqlite3* db, MusicLibrary* library);
int db_load_tracks(sqlite3* db, MusicLibrary* library);

int db_save_artist(sqlite3* db, Artist* artist);
int db_save_album(sqlite3* db, Album* album);
int db_save_track(sqlite3* db, Track* track);

int db_artist_exists(sqlite3* db, const char* name);
int db_album_exists(sqlite3* db, const char* title, const char* artist_name, const char* album_path);
int db_track_exists(sqlite3* db, const char* title, const char* artist_name, const char* album_title, const char* filename);
void sql_err(sqlite3* db, const char* sql);


int db_create_schema(sqlite3* db)
{
        const char* sql_artists =
                "CREATE TABLE IF NOT EXISTS artists ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT NOT NULL);";

        const char* sql_albums =
                "CREATE TABLE IF NOT EXISTS albums ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "title VARCHAR(55) NOT NULL, "
                "year INTEGER, "
                "artist_name VARCHAR(55), "
                "album_path VARCHAR(255) NOT NULL, "
                "art_path VARCHAR(255) NOT NULL, "
                "thumb_image BLOB, "
                "thumb_width INTEGER, "
                "thumb_height INTEGER, "
                "thumb_channels INTEGER, "
                "thumb_bytes INTEGER, "
                "art_image BLOB, "
                "art_width INTEGER, "
                "art_height INTEGER, "
                "art_channels INTEGER, "
                "art_bytes INTEGER);";

        const char* sql_tracks =
                "CREATE TABLE IF NOT EXISTS tracks ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "title VARCHAR(85) NOT NULL, "
                "artist_name VARCHAR(55), "
                "album_title VARCHAR(55), "
                "album_artist_name VARCHAR(55), "
                "album_path VARCHAR(255), "
                "filename VARCHAR(255), "
                "track_number INTEGER, "
                "disc_number INTEGER, "
                "duration INTEGER NOT NULL, "
                "year INTEGER, "
                "has_meta_cover BOOLEAN NOT NULL);";

        char* errmsg = NULL;

        if (sqlite3_exec(db, sql_artists, 0, 0, &errmsg) != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return 1;
        }

        if (sqlite3_exec(db, sql_albums, 0, 0, &errmsg) != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return 1;
        }

        if (sqlite3_exec(db, sql_tracks, 0, 0, &errmsg) != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return 1;
        }

        return 0;
}


int db_load_music_library(sqlite3* db, MusicLibrary* library)
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

        if (db_load_artists(db, library) != 0)
                return -1;
        if (db_load_albums(db, library) != 0)
                return -1;
        if (db_load_tracks(db, library) != 0)
                return -1;
        return 0;
}

int db_save_music_library(sqlite3* db, MusicLibrary* library)
{
        Artist* artist = library->artist_list.head;
        Album* album = library->album_list.head;
        Track* track = library->track_list.head;

        if (sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL) != SQLITE_OK) {
                sql_err(db, "BEGIN TRANSACTION");
                return -1;
        }

        while (artist) {
                if (db_save_artist(db, artist) != 0) {
                        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                        return -1;
                }
                artist = artist->next;
        }

        while (album) {
                if (db_save_album(db, album) != 0) {
                        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                        return -1;
                }
                album = album->next;
        }

        while (track) {
                if (db_save_track(db, track) != 0) {
                        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                        return -1;
                }
                track = track->next;
        }

        if (sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL) != SQLITE_OK) {
                sql_err(db, "END TRANSACTION");
                return -1;
        }

        return 0;
}



int db_load_artists(sqlite3* db, MusicLibrary* library)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT name FROM artists";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
                Artist* artist = malloc(sizeof(Artist));
                artist->name = strdup((const char*)sqlite3_column_text(stmt, 0));
                artist->albums_head = NULL;
                artist->albums_tail = NULL;
                artist->next = NULL;
                artist->prev = NULL;
                artist->album_count = 0;

                append_artist(library, artist);
        }

        sqlite3_finalize(stmt);
        return 0;
}


int db_load_albums(sqlite3* db, MusicLibrary* library)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT title, year, artist_name, album_path, art_path FROM albums";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
                Album* album = malloc(sizeof(Album));
                album->title = strdup((const char*)sqlite3_column_text(stmt, 0));
                album->year = sqlite3_column_int(stmt, 1);
                album->artist_name = strdup((const char*)sqlite3_column_text(stmt, 2));
                album->album_path = strdup((const char*)sqlite3_column_text(stmt, 3));
                album->art_path = strdup((const char*)sqlite3_column_text(stmt, 4));
                album->tracks_head = NULL;
                album->tracks_tail = NULL;
                album->album_artist = NULL;
                album->next = NULL;
                album->prev = NULL;
                album->art.loaded = false;
                album->track_count = 0;

                if (album->art_path && strlen(album->art_path) > 0) {
                        if (load_image_from_file(album->art_path, &album->art) != 0) {
                                album->art.data = NULL;
                        }
                }
                else {
                        album->art.data = NULL;
                }

                append_album(library, album);
        }

        sqlite3_finalize(stmt);
        return 0;
}


int db_load_tracks(sqlite3* db, MusicLibrary* library)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT title, artist_name, album_title, album_artist_name, album_path, filename, track_number, disc_number, duration, year, has_meta_cover FROM tracks";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
                Track* track = malloc(sizeof(Track));
                Album* album;
                Artist* artist;

                track->meta.title = strdup((const char*)sqlite3_column_text(stmt, 0));
                track->meta.artist_name = strdup((const char*)sqlite3_column_text(stmt, 1));
                track->meta.album_title = strdup((const char*)sqlite3_column_text(stmt, 2));
                track->meta.album_artist_name = strdup((const char*)sqlite3_column_text(stmt, 3));
                track->meta.album_path = strdup((const char*)sqlite3_column_text(stmt, 4));
                track->filename = strdup((const char*)sqlite3_column_text(stmt, 5));
                track->meta.track_number = sqlite3_column_int(stmt, 6);
                track->meta.disc_number = sqlite3_column_int(stmt, 7);
                track->duration = sqlite3_column_int(stmt, 8);
                track->meta.year = sqlite3_column_int(stmt, 9);
                track->meta.has_meta_cover = sqlite3_column_int(stmt, 10);
                track->album = NULL;
                track->artist = NULL;
                track->next = NULL;
                track->prev = NULL;

                album = find_album(&library->album_list, track->meta.album_title, track->meta.album_artist_name, track->meta.album_path);
                if (album) {
                        track->album = album;
                        append_track_node(&album->tracks_head, &album->tracks_tail, track, &album->track_count);

                        if (album->art.data == NULL)
                                enqueue_album_cover(&library->album_cover_queue, library, track, album);
                }
                /* TODO various artists
                else if ((strcmp(track->meta.album_artist_name, meta->artist_name) != 0) && album->album_artist != track->artist) {
                                bool is_album_artist_new = false;
                                Artist* album_artist = find_or_create_artist(library, "Various Artists", &is_album_artist_new);
                                if (is_album_artist_new)
                                        append_album_node(&album_artist->albums_head, &album_artist->albums_tail, album);
                                album->album_artist = album_artist;

                }*/
                artist = find_artist(&library->artist_list, track->meta.artist_name);
                if (artist) {
                        AlbumNode* album_node;

                        track->artist = artist;

                        album_node = find_or_create_album_node(artist, album);
                        if (!artist_has_album(artist, album))
                                append_album_node(&artist->albums_head, &artist->albums_tail, album, &artist->album_count);
                }

                append_track(library, track);

                /* find/link album_artist if different from track artist */
                if (track->meta.album_artist_name && strlen(track->meta.album_artist_name) > 0
                && strcmp(track->meta.artist_name, track->meta.album_artist_name) != 0) {
                        Artist* album_artist = find_artist(&library->artist_list, track->meta.album_artist_name);
                        if (album_artist && !artist_has_album(album_artist, album)) {
                                AlbumNode* album_node = find_or_create_album_node(album_artist, album);
                                append_album_node(&album_artist->albums_head, &album_artist->albums_tail, album, &album_artist->album_count);
                        }
                }
        }

        sqlite3_finalize(stmt);
        return 0;
}


int db_save_artist(sqlite3* db, Artist* artist)
{
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO artists (name) VALUES (?)";

        if (db_artist_exists(db, artist->name))
                return 0;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        sqlite3_bind_text(stmt, 1, artist->name, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
                sql_err(db, sql);
                sqlite3_finalize(stmt);
                return -1;
        }

        sqlite3_finalize(stmt);
        return 0;
}


int db_save_album(sqlite3* db, Album* album)
{
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO albums (title, year, artist_name, album_path, art_path) VALUES (?, ?, ?, ?, ?)";

        if (db_album_exists(db, album->title, album->artist_name, album->album_path))
                return 0;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        sqlite3_bind_text(stmt, 1, album->title, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, album->year);
        sqlite3_bind_text(stmt, 3, album->artist_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, album->album_path, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, album->art_path, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
                sql_err(db, sql);
                sqlite3_finalize(stmt);
                return -1;
        }

        sqlite3_finalize(stmt);
        return 0;
}


int db_save_track(sqlite3* db, Track* track)
{
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO tracks (title, artist_name, album_title, album_artist_name, album_path, filename, track_number, disc_number, duration, year, has_meta_cover) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        AudioMetadata* meta = &track->meta;

        if (db_track_exists(db, meta->title, meta->artist_name, meta->album_title, track->filename))
                return 0;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        sqlite3_bind_text(stmt, 1, meta->title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, meta->artist_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, meta->album_title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, meta->album_artist_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, meta->album_path, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, track->filename, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 7, meta->track_number);
        sqlite3_bind_int(stmt, 8, meta->disc_number);
        sqlite3_bind_int(stmt, 9, track->duration);
        sqlite3_bind_int(stmt, 10, meta->year);
        sqlite3_bind_int(stmt, 11, meta->has_meta_cover);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
                sql_err(db, sql);
                sqlite3_finalize(stmt);
                return -1;
        }

        sqlite3_finalize(stmt);
        return 0;
}


int db_artist_exists(sqlite3* db, const char* name)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT 1 FROM artists where name = ?";
        bool exists = false;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

        exists = (sqlite3_step(stmt) == SQLITE_ROW);

        sqlite3_finalize(stmt);
        return exists;
}


int db_album_exists(sqlite3* db, const char* title, const char* artist_name, const char* album_path)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT 1 FROM albums WHERE title = ? AND artist_name = ? AND album_path = ?";
        bool exists = false;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        sqlite3_bind_text(stmt, 1, title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, artist_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, album_path, -1, SQLITE_STATIC);

        exists = sqlite3_step(stmt) == SQLITE_ROW;

        sqlite3_finalize(stmt);
        return exists;
}


int db_track_exists(sqlite3* db, const char* title, const char* artist_name, const char* album_title, const char* filename)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT 1 FROM tracks WHERE title = ? AND artist_name = ? AND album_title = ? AND filename = ?";
        bool exists = false;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return -1;
        }

        sqlite3_bind_text(stmt, 1, title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, artist_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, album_title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, filename, -1, SQLITE_STATIC);

        exists = sqlite3_step(stmt) == SQLITE_ROW;

        sqlite3_finalize(stmt);
        return exists;
}


bool db_file_in_database(sqlite3* db, const char* filename)
{
        sqlite3_stmt* stmt;
        const char* sql = "SELECT 1 FROM tracks WHERE filename = ?";
        bool exists = 0;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                sql_err(db, sql);
                return false;
        }

        sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_STATIC);
        exists = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);

        return exists;
}


void sql_err(sqlite3* db, const char* sql)
{
        fprintf(stderr, "SQL error: %s\n    Query: %s\n", sqlite3_errmsg(db), sql);
}
