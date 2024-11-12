#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT

#include "portamento.h"
#include "nuklear_glfw_gl3.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024
#define ALBUM_SIZE 162
#define FULL_ALBUM_SIZE 300
#define TRACK_ALBUM_SIZE 64
#define ALBUM_MARGIN 10
#define ALBUM_CELL_HEIGHT 55
#define SIZE_LIMIT 760

#define TOP_BAR_HEIGHT 30
#define LEFT_PANEL_WIDTH 200
#define RIGHT_PANEL_WIDTH 350

typedef enum {
        ALBUM_GRID_VIEW,
        TRACK_LIST_VIEW,
        SELECTED_ALBUM_VIEW
} MiddleViewType;

typedef struct {
        struct nk_font* font_small;
        struct nk_font* font_normal;
        struct nk_font* font_large;
        struct nk_font* font_huge;

        struct nk_font* font_normal_bold;
        struct nk_font* font_large_bold;
        struct nk_font* font_huge_bold;

        Image shadow_image;
} NuklearMedia;

typedef struct {
        int w; /* window width */
        int h; /* window height */
        struct nk_context* ctx;
        struct nk_glfw glfw;
        GLFWwindow* glfw_win;
        MiddleViewType middle_view_type;

        NuklearMedia media;

        Artist* selected_artist;
        Album* selected_album;
        Track* selected_track;
        bool settings_open;
} Window;

void handle_events(MusicLibrary* library);
void draw_music_library(MusicLibrary* library);
void render_init(Window* win);
void draw_settings(Window* win, MusicLibrary* library);
void draw_topbar(Window* win, MusicLibrary* library);
void draw_leftpanel(Window* win, MusicLibrary* library);
void draw_middlepanel(Window* win, MusicLibrary* library);
void draw_rightpanel(Window* win, MusicLibrary* library);
int draw_artist_button(Window* win, const char* artist_name, int num_albums, int num_tracks);
void draw_artist_list(Window* win, MusicLibrary* library);
void draw_selected_album(Window* win);
void draw_album(Window* win, Album* album, int cell_size);
void draw_artist_albums(Window* win, Artist* artist, int cell_size);
void draw_albums(Window* win, MusicLibrary* library, int cell_size);
int draw_track_button(Window* win, const char* track_name, const char* album_name, const char* artist_name, int size);
void draw_track(Window* win, Track* track, bool draw_cover_art);
void draw_tracks(Window* win, MusicLibrary* library);
void draw_artist_tracks(Window* win, Artist* artist);
void cleanup(Window* win);
void custom_nuklear_style(struct nk_context* ctx);
void reset_button_style(struct nk_context* ctx);
void set_tab_button_style(struct nk_context* ctx);
void draw_shadow(Window* win, struct nk_rect rect, float shadow_offset);
float get_text_width(struct nk_context* ctx, const char* text);


static void error_callback(int e, const char *d)
{
        fprintf(stderr, "Error %d: %s\n", e, d);
}


void draw_music_library(MusicLibrary* library)
{
        Window win = {0};
        struct nk_colorf bg;

        win.middle_view_type = ALBUM_GRID_VIEW;

        render_init(&win);
        custom_nuklear_style(win.ctx);

        while (!glfwWindowShouldClose(win.glfw_win)) {
                glfwPollEvents();
                nk_glfw3_new_frame(&win.glfw);

                handle_events(library);
                glfwGetWindowSize(win.glfw_win, &win.w, &win.h);

                draw_topbar(&win, library);
                if (win.w > SIZE_LIMIT) {
                        draw_leftpanel(&win, library);
                        draw_middlepanel(&win, library);
                }
                draw_rightpanel(&win, library);
                draw_settings(&win, library);

                glViewport(0, 0, win.w, win.h);
                glClear(GL_COLOR_BUFFER_BIT);
                glClearColor(bg.r, bg.g, bg.b, bg.a);
                nk_glfw3_render(&win.glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
                glfwSwapBuffers(win.glfw_win);
                glfwWaitEventsTimeout(0.016);
        }
        cleanup(&win);
}


void handle_events(MusicLibrary* library)
{
        Event* e = try_dequeue_event(&library->event_queue);
        bool scanning_done = false;

        while (e) {
                switch (e->type) {
                case EVENT_TRACK_LOADED:
                        /*printf("Loaded track %s\n", e->data.track->meta.title);*/
                        break;
                case EVENT_ALBUM_LOADED:
                        /*printf("Loaded album %s\n", e->data.album->title);*/
                        break;
                case EVENT_ARTIST_LOADED:
                        /*printf("Loaded artist %s\n", e->data.artist->name);*/
                        break;
                case EVENT_ALBUM_ART_LOADED:
                        gl_send_image(&e->data.album->art);
                        /*printf("Loaded album art for %s (%d x %d)\n", e->data.album->title, e->data.album->art.width, e->data.album->art.height);*/
                        break;
                }
                free(e);
                e = try_dequeue_event(&library->event_queue);
        }

        pthread_mutex_lock(&library->mutex);
        scanning_done = library->scanning_done;
        pthread_mutex_unlock(&library->mutex);

        if (scanning_done && library->event_queue.head == NULL) {
                pthread_mutex_lock(&library->album_cover_queue.mutex);
                library->album_cover_queue.finished = true;
                pthread_cond_signal(&library->album_cover_queue.cond);
                pthread_mutex_unlock(&library->album_cover_queue.mutex);
        }
}


void render_init(Window* win)
{
        /*int initial_w = 1400;
        int initial_h = 800;*/
        int initial_w = 1366;
        int initial_h = 768;

        float fnt_size_small = 16.0f;
        float fnt_size_normal = 18.0f;
        float fnt_size_large = 28.0f;
        float fnt_size_huge = 42.0f;

        struct nk_font_atlas* atlas;
        struct nk_font_config cfg_small = nk_font_config(fnt_size_small);
        struct nk_font_config cfg_normal = nk_font_config(fnt_size_normal);
        struct nk_font_config cfg_large = nk_font_config(fnt_size_large);
        Image* shadow_img = &win->media.shadow_image;

        glfwSetErrorCallback(error_callback);
        if (!glfwInit()) {
                fprintf(stderr, "Failed to initialize GLFW\n");
                glfwTerminate();
                return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        win->glfw_win = glfwCreateWindow(initial_w, initial_h, "Portamento", NULL, NULL);
        glfwMakeContextCurrent(win->glfw_win);
        glfwSetWindowUserPointer(win->glfw_win, &win->ctx);
        glfwGetWindowSize(win->glfw_win, &win->w, &win->h);

        glewExperimental = 1;
        if (glewInit() != GLEW_OK) {
                fprintf(stderr, "Failed to setup GLEW\n");
                glfwDestroyWindow(win->glfw_win);
                glfwTerminate();
                exit(1);
        }

        glViewport(0, 0, initial_w, initial_h);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glfwSwapInterval(1);

        win->ctx = nk_glfw3_init(&win->glfw, win->glfw_win, NK_GLFW3_INSTALL_CALLBACKS);

        cfg_small.oversample_h = 4;
        cfg_small.oversample_v = 4;
        cfg_normal.oversample_h = 4;
        cfg_normal.oversample_v = 4;
        cfg_large.oversample_h = 4;
        cfg_large.oversample_v = 4;

        nk_glfw3_font_stash_begin(&win->glfw, &atlas);
        /*win->media.font_normal = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18.0f, 0);
        win->media.font_large = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28.0f, 0);*/
        win->media.font_small = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Light.ttf", fnt_size_small, &cfg_small);
        win->media.font_normal = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", fnt_size_normal, &cfg_normal);
        win->media.font_large = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", fnt_size_large, &cfg_large);
        win->media.font_huge = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", fnt_size_huge, &cfg_large);

        win->media.font_normal_bold = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", fnt_size_normal, &cfg_normal);
        win->media.font_large_bold = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", fnt_size_large, &cfg_large);
        win->media.font_huge_bold = nk_font_atlas_add_from_file(atlas, "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", fnt_size_huge, &cfg_large);

        if (!win->media.font_normal
        || !win->media.font_small
        || !win->media.font_large
        || !win->media.font_normal_bold
        || !win->media.font_large_bold) {
                fprintf(stderr, "Failed to load font\n");
                win->media.font_small = nk_font_atlas_add_default(atlas, fnt_size_small, 0);
                win->media.font_normal = nk_font_atlas_add_default(atlas, fnt_size_normal, 0);
                win->media.font_large = nk_font_atlas_add_default(atlas, fnt_size_large, 0);
                win->media.font_normal_bold = nk_font_atlas_add_default(atlas, fnt_size_normal, 0);
                win->media.font_large_bold = nk_font_atlas_add_default(atlas, fnt_size_large, 0);
                win->media.font_huge_bold = nk_font_atlas_add_default(atlas, fnt_size_huge, 0);
        }

        nk_glfw3_font_stash_end(&win->glfw);

        nk_style_set_font(win->ctx, &win->media.font_normal->handle);

        shadow_img->data = stbi_load("/usr/local/share/portamento/res/shadow.png", &shadow_img->width, &shadow_img->height, &shadow_img->channels, 0);
        /*image_resize_and_compress(shadow_img, 184, 184);*/
        gl_send_uncompressed_image(shadow_img);

        win->selected_artist = NULL;
        win->selected_album = NULL;
        win->selected_track = NULL;
        win->settings_open = NULL;
}


void draw_settings(Window* win, MusicLibrary* library)
{
        struct nk_context* ctx = win->ctx;

        int w = win->w / 2;
        int h = win->h / 2;
        int x = (win->w / 2) - (w / 2);
        int y = (win->h / 2) - (h / 2);

        if (!win->settings_open)
                return;

        if (nk_begin(ctx, "Settings", nk_rect(x, y, w, h), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE | NK_WINDOW_SCALABLE)) {
                nk_layout_row_dynamic(ctx, 50, 1);

                if (nk_button_label(ctx, "Close"))
                        win->settings_open = false;
        }
        else
                win->settings_open = false;

        nk_end(ctx);


}


void draw_topbar(Window* win, MusicLibrary* library)
{
        struct nk_context* ctx = win->ctx;
        float menu_width = get_text_width(ctx, "Refresh Music Library") + 20;
        if (nk_begin(ctx, "TopBar", nk_rect(0, 0, win->w, TOP_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR)) {
                nk_menubar_begin(ctx);
                nk_layout_row_static(ctx, TOP_BAR_HEIGHT, 80, 3);
                if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(menu_width, 200))) {
                        nk_layout_row_dynamic(ctx, 25, 1);
                        nk_menu_item_label(ctx, "Refresh Music Library", NK_TEXT_LEFT);
                        nk_menu_item_label(ctx, "Rebuild Music Library", NK_TEXT_LEFT);
                        if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT))
                                win->settings_open = !win->settings_open;
                        nk_menu_end(ctx);
                }
                nk_menubar_end(ctx);
        }
        nk_end(ctx);
}


void draw_leftpanel(Window* win, MusicLibrary* library)
{
        struct nk_context* ctx = win->ctx;
        int tab_height = 30;
        int w = LEFT_PANEL_WIDTH;
        int h = win->h - TOP_BAR_HEIGHT - tab_height;

        if (nk_begin(ctx, "Left Panel Tabs", nk_rect(0, TOP_BAR_HEIGHT, w, tab_height), NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_static(ctx, tab_height, 65, 2);

                set_tab_button_style(ctx);
                if(nk_button_label(ctx, "Artists")) {
                }
                if(nk_button_label(ctx, "Playlists")) {
                }
                reset_button_style(ctx);
        }
        nk_end(ctx);

        if (nk_begin(ctx, "LeftPanel", nk_rect(0, TOP_BAR_HEIGHT + tab_height, w, h), NK_WINDOW_BORDER)) {
                draw_artist_list(win, library);
        }
        nk_end(ctx);
}


void draw_middlepanel(Window* win, MusicLibrary* library)
{
        struct nk_context* ctx = win->ctx;
        int tab_height = 30;
        int w = win->w - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH;
        int h = win->h - TOP_BAR_HEIGHT - tab_height;

        if (nk_begin(ctx, "Middel Panel Tabs", nk_rect(LEFT_PANEL_WIDTH, TOP_BAR_HEIGHT, w, tab_height), NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_static(ctx, tab_height, 65, 2);

                set_tab_button_style(ctx);
                if(nk_button_label(ctx, "Albums")) {
                        win->middle_view_type = ALBUM_GRID_VIEW;
                }
                if(nk_button_label(ctx, "Tracks")) {
                        win->middle_view_type = TRACK_LIST_VIEW;
                }
                reset_button_style(ctx);
        }
        nk_end(ctx);

        if (nk_begin(ctx, "Middle Panel", nk_rect(LEFT_PANEL_WIDTH, TOP_BAR_HEIGHT + tab_height, w, h), NK_WINDOW_BORDER)) {
                switch (win->middle_view_type) {
                case ALBUM_GRID_VIEW:
                        w = w - 8; /* account for scrollbar taking up some of the width */
                        /* (the 4's come from the fact that the elements seems to have an additional 4px margin) */
                        float columns = (w - ((ALBUM_SIZE + 4)/ 2)) / (ALBUM_SIZE + 4);
                        float total_album_width = columns * (ALBUM_SIZE + 4);
                        float remaining_space = w - total_album_width;
                        float dynamic_margin = remaining_space / (columns + 1);
                        float cell_size = ALBUM_SIZE + dynamic_margin;

                        if (win->selected_artist) {
                                /* Draw artist name */
                                int padding = (dynamic_margin / 2) + 7;
                                float text_size = 0.0f;

                                nk_style_set_font(win->ctx, &win->media.font_large->handle);
                                text_size = get_text_width(ctx, win->selected_artist->name);

                                nk_layout_space_begin(ctx, NK_STATIC, 40, 1);
                                nk_layout_space_push(ctx, nk_rect(padding, 0, text_size, 40));

                                nk_label(ctx, win->selected_artist->name, NK_TEXT_LEFT);

                                nk_style_set_font(win->ctx, &win->media.font_normal->handle);

                                nk_layout_space_end(ctx);

                                /* grid layout */
                                nk_layout_row_static(ctx, ALBUM_SIZE + ALBUM_CELL_HEIGHT, cell_size, columns);
                                draw_artist_albums(win, win->selected_artist, cell_size);
                        }
                        else {
                                /* grid layout */
                                nk_layout_row_static(ctx, ALBUM_SIZE + ALBUM_CELL_HEIGHT, cell_size, columns);
                                draw_albums(win, library, cell_size);
                        }
                        break;
                case TRACK_LIST_VIEW:
                        if (win->selected_artist)
                                draw_artist_tracks(win, win->selected_artist);
                        else
                                draw_tracks(win, library);
                        break;
                case SELECTED_ALBUM_VIEW:
                        if (win->selected_album)
                                draw_selected_album(win);
                        else
                                win->middle_view_type = ALBUM_GRID_VIEW;
                        break;
                }
        }
        nk_end(ctx);
}


void draw_rightpanel(Window* win, MusicLibrary* library)
{
        struct nk_context* ctx = win->ctx;
        int w = (win->w < SIZE_LIMIT) ? win->w : RIGHT_PANEL_WIDTH;
        int h = win->h - TOP_BAR_HEIGHT;
        int x = (win->w < SIZE_LIMIT) ? 0 : win->w - RIGHT_PANEL_WIDTH;

        int shadow_margin = 8;
        int shadow_x = (w - (FULL_ALBUM_SIZE + (shadow_margin / 2))) / 2;
        int album_x = (w - FULL_ALBUM_SIZE) / 2;
        int album_y = 0;/*(h - FULL_ALBUM_SIZE) / 2;*/
        int top_padding = 50;

        if (nk_begin(ctx, "RightPanel", nk_rect(x, TOP_BAR_HEIGHT, w, h), NK_WINDOW_NO_SCROLLBAR)) {
                if (!win->selected_track)
                        goto rightpanelend;

                struct nk_rect art_bounds = nk_rect(x + shadow_x, TOP_BAR_HEIGHT + album_y + top_padding, FULL_ALBUM_SIZE + shadow_margin, FULL_ALBUM_SIZE + 10);
                draw_shadow(win, art_bounds, 0.0f);

                nk_layout_row_dynamic(ctx, top_padding, 1);

                if (win->selected_track->album
                && win->selected_track->album->art.loaded) {
                        nk_layout_space_begin(ctx, NK_STATIC, FULL_ALBUM_SIZE, 1);
                        nk_layout_space_push(ctx, nk_rect(album_x, album_y, FULL_ALBUM_SIZE, FULL_ALBUM_SIZE));
                        nk_image(ctx, win->selected_track->album->art.nk_img);
                        nk_layout_space_end(ctx);
                }
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_style_set_font(ctx, &win->media.font_large_bold->handle);
                nk_label(ctx, win->selected_track->meta.title, NK_TEXT_CENTERED);

                nk_style_set_font(ctx, &win->media.font_normal->handle);

                nk_layout_row_dynamic(ctx, 25, 1);
                nk_label(ctx, win->selected_track->meta.artist_name, NK_TEXT_CENTERED);

                nk_layout_row_dynamic(ctx, 25, 1);
                nk_label(ctx, win->selected_track->meta.album_title, NK_TEXT_CENTERED);

                nk_style_set_font(ctx, &win->media.font_normal->handle);
        }
rightpanelend:
        nk_end(ctx);
}


int draw_artist_button(Window* win, const char* artist_name, int num_albums, int num_tracks)
{
        struct nk_context* ctx = win->ctx;
        nk_layout_row_dynamic(ctx, 75, 1);
        struct nk_rect bounds = nk_widget_bounds(ctx);
        struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
        int click = 0;

        if (nk_group_begin(ctx, "Artist Button", NK_WINDOW_NO_SCROLLBAR)) {
                char str_album_count[32];
                char str_track_count[32];

                sprintf(str_album_count, "Albums: %d", num_albums);
                sprintf(str_track_count, "Tracks: %d", num_tracks);

                if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds)) {
                        nk_fill_rect(canvas, bounds, 0, nk_rgb(200, 200, 200));
                        click = nk_input_is_mouse_pressed(&ctx->input, NK_BUTTON_LEFT);
                }

                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, artist_name, NK_TEXT_LEFT);

                nk_style_set_font(ctx, &win->media.font_small->handle);
                nk_layout_row_dynamic(ctx, 15, 1);
                nk_label(ctx, str_album_count, NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 15, 1);
                nk_label(ctx, str_track_count, NK_TEXT_LEFT);
                nk_style_set_font(ctx, &win->media.font_normal->handle);

                nk_group_end(ctx);
        }
        return click;
}


void draw_artist_list(Window* win, MusicLibrary* library)
{
        struct nk_context* ctx = win->ctx;
        Artist* current = library->artist_list.head;

        if (draw_artist_button(win, "All Artists", library->album_list.count, library->track_list.count)) {
                win->selected_artist = NULL;
                win->selected_album = NULL;
                if (win->middle_view_type == SELECTED_ALBUM_VIEW)
                        win->middle_view_type = ALBUM_GRID_VIEW;
        }

        while (current) {
                if (draw_artist_button(win, current->name, current->album_count, get_artist_track_count(current))) {
                        win->selected_artist = current;
                        win->selected_album = NULL;
                        if (win->middle_view_type == SELECTED_ALBUM_VIEW)
                                win->middle_view_type = ALBUM_GRID_VIEW;
                }
                current = current->next;
        }
}


void draw_selected_album(Window* win)
{
        Album* album = win->selected_album;
        TrackNode* current = album->tracks_head;
        struct nk_context* ctx = win->ctx;
        int curdisc = 0;
        float title_width;
        float artist_width;
        float textbox_width;
        const char* artist_name = get_album_artist_name(album);

        nk_layout_row_static(ctx, 25, 50, 1);
        if (nk_button_label(ctx, "back")) {
                win->selected_album = NULL;
                win->middle_view_type = ALBUM_GRID_VIEW;
        }

        nk_layout_row_begin(ctx, NK_STATIC, ALBUM_SIZE / 3, 2);
        nk_layout_row_push(ctx, ALBUM_SIZE / 3);
        if (ctx, album->art.loaded) {
                struct nk_rect art_bounds = nk_widget_bounds(ctx);
                draw_shadow(win, art_bounds, 0.0f);
                nk_image(ctx, album->art.nk_img);
        }
        else
                nk_label(ctx, "?", NK_TEXT_CENTERED);

        artist_width = get_text_width(ctx, artist_name);

        nk_style_set_font(ctx, &win->media.font_large->handle);
        title_width = get_text_width(ctx, album->title);

        textbox_width = (artist_width > title_width) ? artist_width : title_width;

        nk_layout_row_push(ctx, textbox_width + 25);
        if (nk_group_begin(ctx, album->title, NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, album->title, NK_TEXT_LEFT);
                nk_style_set_font(ctx, &win->media.font_normal->handle);
                nk_label(ctx, get_album_artist_name(album), NK_TEXT_LEFT);
                nk_group_end(ctx);
        }
        nk_style_set_font(ctx, &win->media.font_normal->handle);
        /*nk_layout_row_end(ctx);*/

        while (current) {
                if (current->track->meta.disc_number != curdisc && current->track->meta.disc_number != 0) {
                        char str[16];
                        curdisc = current->track->meta.disc_number;
                        sprintf(str, "Disc %d", curdisc);
                        nk_layout_row_static(ctx, 32, 50, 1);
                        nk_label(ctx, str, NK_TEXT_LEFT);
                }
                draw_track(win, current->track, false);
                current = current->next;
        }
}


void draw_album(Window* win, Album* album, int cell_size)
{
        struct nk_context* ctx = win->ctx;
        if (!nk_group_begin(ctx, album->title, NK_WINDOW_NO_SCROLLBAR)) {
                return;
        }
        int padding = ((cell_size - ALBUM_SIZE) / 2) - 1;

        nk_layout_space_begin(ctx, NK_STATIC, ALBUM_SIZE, 1);
        nk_layout_space_push(ctx, nk_rect(padding, 0, ALBUM_SIZE, ALBUM_SIZE));
        struct nk_rect art_bounds = nk_widget_bounds(ctx);
        draw_shadow(win, art_bounds, 0.0f);

        if (album->art.loaded) {
                if (nk_button_image(ctx, album->art.nk_img)) {
                        win->selected_album = album;
                        win->middle_view_type = SELECTED_ALBUM_VIEW;
                }
        }
        else if (nk_button_label(ctx, "No album art")) {
                win->selected_album = album;
                win->middle_view_type = SELECTED_ALBUM_VIEW;
        }
        nk_layout_space_push(ctx, nk_rect(padding, 0, ALBUM_SIZE, ALBUM_SIZE));
        nk_layout_space_end(ctx);

        nk_layout_row_static(ctx, 20, cell_size, 1);
        nk_style_set_font(win->ctx, &win->media.font_normal->handle);
        nk_label(ctx, album->title, NK_TEXT_CENTERED);
        nk_style_set_font(ctx, &win->media.font_small->handle);

        if (strcmp(album->artist_name, "") != 0)
                nk_label(ctx, album->artist_name, NK_TEXT_CENTERED);
        else if (album->album_artist)
                nk_label(ctx, album->album_artist->name, NK_TEXT_CENTERED);
        else
                nk_label(ctx, album->tracks_head->track->meta.artist_name, NK_TEXT_CENTERED);

        nk_style_set_font(win->ctx, &win->media.font_normal->handle);

        nk_group_end(ctx);

}



void draw_artist_albums(Window* win, Artist* artist, int cell_size)
{
        AlbumNode* current = artist->albums_head;
        while (current) {
                draw_album(win, current->album, cell_size);
                current = current->next;
        }
}


void draw_albums(Window* win, MusicLibrary* library, int cell_size)
{
        Album* album = library->album_list.head;
        while (album) {
                draw_album(win, album, cell_size);
                album = album->next;
        }
}


int draw_track_button(Window* win, const char* track_name, const char* album_name, const char* artist_name, int size)
{
        struct nk_context* ctx = win->ctx;
        struct nk_rect bounds = nk_widget_bounds(ctx);
        struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
        int click = 0;

        if (nk_group_begin(ctx, "Artist Button", NK_WINDOW_NO_SCROLLBAR)) {
                if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds)) {
                        nk_fill_rect(canvas, bounds, 0, nk_rgb(200, 200, 200));
                        click = nk_input_is_mouse_pressed(&ctx->input, NK_BUTTON_LEFT);
                }

                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, track_name, NK_TEXT_LEFT);

                nk_style_set_font(ctx, &win->media.font_small->handle);
                nk_layout_row_dynamic(ctx, 14, 1);
                nk_label(ctx, album_name, NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 14, 1);
                nk_label(ctx, artist_name, NK_TEXT_LEFT);
                nk_style_set_font(ctx, &win->media.font_normal->handle);

                nk_group_end(ctx);
        }
        return click;
}


void draw_track(Window* win, Track* track, bool draw_cover_art)
{
        struct nk_context* ctx = win->ctx;
        struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
        int w = win->w - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH;
        int size = draw_cover_art ? TRACK_ALBUM_SIZE : 28;
        int indent_size = draw_cover_art ? 0 : 16;

        nk_layout_row_begin(ctx, NK_STATIC, size, 4);

        nk_layout_row_push(ctx, indent_size);
        nk_label(ctx, "", NK_TEXT_LEFT);

        nk_layout_row_push(ctx, size);
        if (track->album->art.loaded && draw_cover_art) {
                struct nk_rect art_bounds = nk_widget_bounds(ctx);
                draw_shadow(win, art_bounds, 0.0f);
                nk_image(ctx, track->album->art.nk_img);
        }
        else if (draw_cover_art)
                nk_label(ctx, "?", NK_TEXT_CENTERED);
        else {
                char str[32];
                sprintf(str, "%02d  ", track->meta.track_number);
                nk_label(ctx, str, NK_TEXT_LEFT);
        }

        nk_layout_row_push(ctx, w - size - 100);
        /*nk_layout_row_dynamic(ctx, size, 1);*/
        if (draw_cover_art) {
                if (draw_track_button(win, track->meta.title, track->meta.album_title, track->meta.artist_name, size)) {
                        win->selected_track = track;
                }
        }
        else if (nk_button_label(ctx, track->meta.title)) {
                win->selected_track = track;
        }

        nk_layout_row_push(ctx, 50);
        nk_label(ctx, "0:00", NK_TEXT_LEFT);
}


void draw_tracks(Window* win, MusicLibrary* library)
{
        Track* track = library->track_list.head;
        int disc_number = 0;
        while (track) {
                draw_track(win, track, true);
                track = track->next;
        }
}


void draw_artist_tracks(Window* win, Artist* artist)
{
        AlbumNode* current_album = artist->albums_head;
        while (current_album) {
                TrackNode* current_track = current_album->album->tracks_head;
                while (current_track) {
                        draw_track(win, current_track->track, true);
                        current_track = current_track->next;
                }
                current_album = current_album->next;
        }
}


void cleanup(Window* win)
{
        nk_glfw3_shutdown(&win->glfw);
        glfwTerminate();
}


void custom_nuklear_style(struct nk_context* ctx)
{
        struct nk_color table[NK_COLOR_COUNT];
        struct nk_style_button* style = &ctx->style.button;

        table[NK_COLOR_TEXT] = nk_rgba(0, 0, 0, 255);
        /*table[NK_COLOR_WINDOW] = nk_rgba(240, 240, 240, 255);*/
        table[NK_COLOR_WINDOW] = nk_rgba(255, 255, 255, 255);
        table[NK_COLOR_HEADER] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_BORDER] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(0, 0, 0, 0);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_SELECT] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(150, 150, 150, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_EDIT] = nk_rgba(200, 200, 200, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_COMBO] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_CHART] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(210, 210, 210, 255);
        nk_style_from_table(ctx, table);
        reset_button_style(ctx);
}


void reset_button_style(struct nk_context* ctx)
{
        struct nk_style_button* style = &ctx->style.button;
        style->border = 0.0f;
        style->text_alignment = NK_TEXT_LEFT;
        style->padding = nk_vec2(0, 0);
        style->touch_padding = nk_vec2(0, 0);
}


void set_tab_button_style(struct nk_context* ctx)
{
        struct nk_style_button* style = &ctx->style.button;
        style->border = 1.0f;
        style->text_alignment = NK_TEXT_LEFT;
}


void draw_shadow(Window* win, struct nk_rect rect, float shadow_offset)
{
        struct nk_command_buffer* cmd = nk_window_get_canvas(win->ctx);

        float shadow_inset = 5.0f;
        struct nk_rect shadow_rect = nk_rect(
                rect.x - shadow_inset + shadow_offset,
                rect.y - shadow_inset + shadow_offset,
                rect.w + 2 * shadow_inset,
                rect.h + 2 * shadow_inset
        );

        if (win->media.shadow_image.loaded) {
                nk_draw_image(cmd, shadow_rect, &win->media.shadow_image.nk_img, nk_rgba(255, 255, 255, 255));
        }
}


float get_text_width(struct nk_context* ctx, const char* text)
{
        struct nk_user_font* font = ctx->style.font;
        float text_width = font->width(font->userdata, font->height, text, nk_strlen(text));
        return text_width;
}
