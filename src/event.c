#include "portamento.h"

void init_event_queue(EventQueue* queue)
{
        queue->head = NULL;
        queue->tail = NULL;
        pthread_mutex_init(&queue->mutex, NULL);
        pthread_cond_init(&queue->cond, NULL);
}


void free_event(Event* event)
{
        free(event);
}


void free_event_queue(EventQueue* queue)
{
        Event* current;
        Event* next;
        pthread_mutex_lock(&queue->mutex);

        current = queue->head;

        while (current) {
                next = current->next;
                free_event(current);
                current = next;
        }

        queue->head = NULL;
        queue->tail = NULL;

        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);

}


void enqueue_event(EventQueue* queue, Event* event)
{
        pthread_mutex_lock(&queue->mutex);
        event->next = NULL;

        if (queue->tail)
                queue->tail->next = event;
        else
                queue->head = event;
        queue->tail = event;
        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
}


Event* try_dequeue_event(EventQueue* queue)
{
        pthread_mutex_lock(&queue->mutex);
        Event* event = queue->head;
        if (event) {
                queue->head = event->next;
                if (!queue->head)
                        queue->tail = NULL;
        }
        pthread_mutex_unlock(&queue->mutex);
        return event;
}


void emit_track_loaded_event(MusicLibrary* library, Track* track)
{
        Event* event = malloc(sizeof(Event));
        event->type = EVENT_TRACK_LOADED;
        event->data.track = track;
        enqueue_event(&library->event_queue, event);
}


void emit_album_loaded_event(MusicLibrary* library, Album* album)
{
        Event* event = malloc(sizeof(Event));
        event->type = EVENT_ALBUM_LOADED;
        event->data.album = album;
        enqueue_event(&library->event_queue, event);
}


void emit_artist_loaded_event(MusicLibrary* library, Artist* artist)
{
        Event* event = malloc(sizeof(Event));
        event->type = EVENT_ARTIST_LOADED;
        event->data.artist = artist;
        enqueue_event(&library->event_queue, event);
}


void emit_album_art_loaded_event(MusicLibrary* library, Album* album)
{
        Event* event = malloc(sizeof(Event));
        event->type = EVENT_ALBUM_ART_LOADED;
        event->data.album = album;
        enqueue_event(&library->event_queue, event);
}
