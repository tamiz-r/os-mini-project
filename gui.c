#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    GtkWidget *window, *list, *path_entry, *status;
    gchar *current_path;
} App;

typedef struct {
    App *app;
    gchar *action;
    gchar *old_name;
} PromptData;

typedef struct {
    App *app;
    gchar *filepath;
    GtkWidget *text_view;
} EditorData;

static void refresh_files(App *app);
static void show_editor(App *app, const gchar *filepath);
static void show_prompt(App *app, const gchar *action,
                        const gchar *old_name);

static void set_status(App *app, const gchar *message)
{
    gtk_label_set_text(GTK_LABEL(app->status), message);
}

static gchar *full_path(App *app, const gchar *name)
{
    return g_build_filename(app->current_path, name, NULL);
}

static gchar *selected_name(App *app)
{
    GtkListBoxRow *row = gtk_list_box_get_selected_row(
        GTK_LIST_BOX(app->list));

    if (!row)
        return NULL;

    return g_strdup(g_object_get_data(
        G_OBJECT(row), "filename"));
}

static void refresh_files(App *app)
{
    GtkWidget *child;

    while ((child = gtk_widget_get_first_child(app->list)))
        gtk_list_box_remove(GTK_LIST_BOX(app->list), child);

    GDir *dir = g_dir_open(app->current_path, 0, NULL);

    if (!dir) {
        set_status(app, "Cannot open directory.");
        return;
    }

    const gchar *name;

    while ((name = g_dir_read_name(dir))) {
        gchar *path = full_path(app, name);
        gboolean is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(NULL);
        gchar *text = g_strdup_printf("%s  %s",
            is_dir ? "📁" : "📄", name);

        gtk_label_set_text(GTK_LABEL(label), text);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_widget_set_margin_start(label, 10);
        gtk_widget_set_margin_end(label, 10);
        gtk_widget_set_margin_top(label, 6);
        gtk_widget_set_margin_bottom(label, 6);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);

        g_object_set_data_full(
            G_OBJECT(row), "filename", g_strdup(name), g_free);
        g_object_set_data(
            G_OBJECT(row), "is-dir", GINT_TO_POINTER(is_dir));

        gtk_list_box_append(GTK_LIST_BOX(app->list), row);

        g_free(text);
        g_free(path);
    }

    g_dir_close(dir);

    gtk_editable_set_text(
        GTK_EDITABLE(app->path_entry), app->current_path);

    set_status(app, "Directory loaded.");
}

static void editor_response(GtkDialog *dialog, int response,
                            gpointer user_data)
{
    EditorData *data = user_data;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(data->text_view));

        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buffer, &start, &end);

        gchar *text = gtk_text_buffer_get_text(
            buffer, &start, &end, FALSE);

        GError *error = NULL;

        if (g_file_set_contents(data->filepath, text, -1, &error)) {
            set_status(data->app, "File saved successfully.");
        } else {
            set_status(data->app, error->message);
            g_error_free(error);
        }

        g_free(text);
        refresh_files(data->app);
    }

    g_free(data->filepath);
    g_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void show_editor(App *app, const gchar *filepath)
{
    gchar *contents = NULL;

    if (!g_file_get_contents(filepath, &contents, NULL, NULL))
        contents = g_strdup("");

    GtkWidget *dialog = gtk_dialog_new();

    gtk_window_set_title(GTK_WINDOW(dialog), "Text Editor");
    gtk_window_set_transient_for(
        GTK_WINDOW(dialog), GTK_WINDOW(app->window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 650, 450);

    gtk_dialog_add_button(
        GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(
        GTK_DIALOG(dialog), "Save", GTK_RESPONSE_ACCEPT);

    GtkWidget *content = gtk_dialog_get_content_area(
        GTK_DIALOG(dialog));

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_margin_start(scrolled, 10);
    gtk_widget_set_margin_end(scrolled, 10);
    gtk_widget_set_margin_top(scrolled, 10);
    gtk_widget_set_margin_bottom(scrolled, 10);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(
        GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

    gtk_text_buffer_set_text(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view)),
        contents, -1);

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scrolled), text_view);
    gtk_box_append(GTK_BOX(content), scrolled);

    EditorData *data = g_new0(EditorData, 1);
    data->app = app;
    data->filepath = g_strdup(filepath);
    data->text_view = text_view;

    g_signal_connect(dialog, "response",
                     G_CALLBACK(editor_response), data);

    g_free(contents);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void prompt_response(GtkDialog *dialog, int response,
                            gpointer user_data)
{
    PromptData *data = user_data;
    App *app = data->app;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkWidget *entry = g_object_get_data(
            G_OBJECT(dialog), "entry");
        const gchar *name = gtk_editable_get_text(
            GTK_EDITABLE(entry));

        if (!name || !*name || strchr(name, '/')) {
            set_status(app, "Invalid name.");
        } else {
            gchar *new_path = full_path(app, name);
            gchar *old_path = data->old_name
                ? full_path(app, data->old_name) : NULL;
            gboolean ok = FALSE;

            if (g_str_equal(data->action, "New File")) {
                FILE *f = g_fopen(new_path, "wx");
                if (f) {
                    fclose(f);
                    ok = TRUE;
                }
            } else if (g_str_equal(data->action, "New Folder")) {
                ok = (g_mkdir(new_path, 0755) == 0);
            } else if (g_str_equal(data->action, "Rename")) {
                ok = (g_rename(old_path, new_path) == 0);
            }

            if (ok)
                set_status(app, "Operation completed.");
            else
                set_status(app, "Operation failed. Check name or permissions.");

            g_free(new_path);
            g_free(old_path);
            refresh_files(app);
        }
    }

    g_free(data->action);
    g_free(data->old_name);
    g_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void show_prompt(App *app, const gchar *action,
                        const gchar *old_name)
{
    GtkWidget *dialog = gtk_dialog_new();

    gtk_window_set_title(GTK_WINDOW(dialog), action);
    gtk_window_set_transient_for(
        GTK_WINDOW(dialog), GTK_WINDOW(app->window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    gtk_dialog_add_button(
        GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(
        GTK_DIALOG(dialog), "OK", GTK_RESPONSE_ACCEPT);

    GtkWidget *content = gtk_dialog_get_content_area(
        GTK_DIALOG(dialog));

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(
        GTK_ENTRY(entry), "Enter name");

    if (old_name)
        gtk_editable_set_text(
            GTK_EDITABLE(entry), old_name);

    gtk_widget_set_margin_start(entry, 12);
    gtk_widget_set_margin_end(entry, 12);
    gtk_widget_set_margin_top(entry, 12);
    gtk_widget_set_margin_bottom(entry, 12);

    gtk_box_append(GTK_BOX(content), entry);
    g_object_set_data(G_OBJECT(dialog), "entry", entry);

    PromptData *data = g_new0(PromptData, 1);
    data->app = app;
    data->action = g_strdup(action);
    data->old_name = g_strdup(old_name);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(prompt_response), data);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void create_file(GtkButton *button, gpointer user_data)
{
    show_prompt(user_data, "New File", NULL);
}

static void create_folder(GtkButton *button, gpointer user_data)
{
    show_prompt(user_data, "New Folder", NULL);
}

static void rename_item(GtkButton *button, gpointer user_data)
{
    App *app = user_data;
    gchar *name = selected_name(app);

    if (!name) {
        set_status(app, "Select an item first.");
        return;
    }

    show_prompt(app, "Rename", name);
    g_free(name);
}

static void delete_item(GtkButton *button, gpointer user_data)
{
    App *app = user_data;
    gchar *name = selected_name(app);

    if (!name) {
        set_status(app, "Select an item first.");
        return;
    }

    gchar *path = full_path(app, name);
    gboolean is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);
    gint result = is_dir ? g_rmdir(path) : g_remove(path);

    if (result == 0)
        set_status(app, "Item deleted.");
    else
        set_status(app, is_dir
            ? "Could not delete folder. It must be empty."
            : "Could not delete file.");

    g_free(name);
    g_free(path);
    refresh_files(app);
}

static void edit_item(GtkButton *button, gpointer user_data)
{
    App *app = user_data;
    gchar *name = selected_name(app);

    if (!name) {
        set_status(app, "Select a file first.");
        return;
    }

    gchar *path = full_path(app, name);

    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        set_status(app, "Select a file, not a folder.");
    } else {
        show_editor(app, path);
    }

    g_free(path);
    g_free(name);
}

static void open_selected(GtkListBox *list, GtkListBoxRow *row,
                          gpointer user_data)
{
    App *app = user_data;
    const gchar *name = g_object_get_data(
        G_OBJECT(row), "filename");

    if (!name)
        return;

    gchar *path = full_path(app, name);

    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        g_free(app->current_path);
        app->current_path = g_strdup(path);
        refresh_files(app);
    } else {
        show_editor(app, path);
    }

    g_free(path);
}

static void go_up(GtkButton *button, gpointer user_data)
{
    App *app = user_data;
    gchar *parent = g_path_get_dirname(app->current_path);

    if (g_strcmp0(parent, app->current_path) != 0) {
        g_free(app->current_path);
        app->current_path = parent;
        refresh_files(app);
    } else {
        g_free(parent);
    }
}

static void go_to_path(GtkButton *button, gpointer user_data)
{
    App *app = user_data;
    const gchar *path = gtk_editable_get_text(
        GTK_EDITABLE(app->path_entry));

    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        g_free(app->current_path);
        app->current_path = g_canonicalize_filename(path, NULL);
        refresh_files(app);
    } else {
        set_status(app, "Directory does not exist.");
    }
}

static void refresh_clicked(GtkButton *button, gpointer user_data)
{
    refresh_files(user_data);
}

static GtkWidget *make_button(const gchar *text, GtkWidget *box,
                              GCallback callback, App *app)
{
    GtkWidget *button = gtk_button_new_with_label(text);
    gtk_box_append(GTK_BOX(box), button);
    g_signal_connect(button, "clicked", callback, app);
    return button;
}

static void activate(GtkApplication *application, gpointer user_data)
{
    App *app = user_data;

    app->current_path = g_get_current_dir();

    app->window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(app->window),
                         "File System Interface");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 900, 600);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_box, 12);
    gtk_widget_set_margin_end(main_box, 12);
    gtk_widget_set_margin_top(main_box, 12);
    gtk_widget_set_margin_bottom(main_box, 12);

    gtk_window_set_child(GTK_WINDOW(app->window), main_box);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(main_box), toolbar);

    make_button("New File", toolbar, G_CALLBACK(create_file), app);
    make_button("New Folder", toolbar, G_CALLBACK(create_folder), app);
    make_button("Edit", toolbar, G_CALLBACK(edit_item), app);
    make_button("Rename", toolbar, G_CALLBACK(rename_item), app);
    make_button("Delete", toolbar, G_CALLBACK(delete_item), app);
    make_button("Up", toolbar, G_CALLBACK(go_up), app);
    make_button("Refresh", toolbar, G_CALLBACK(refresh_clicked), app);

    GtkWidget *path_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(main_box), path_box);

    app->path_entry = gtk_entry_new();
    gtk_widget_set_hexpand(app->path_entry, TRUE);
    gtk_box_append(GTK_BOX(path_box), app->path_entry);

    make_button("Go", path_box, G_CALLBACK(go_to_path), app);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(main_box), scrolled);

    app->list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(
        GTK_LIST_BOX(app->list), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scrolled), app->list);

    g_signal_connect(app->list, "row-activated",
                     G_CALLBACK(open_selected), app);

    app->status = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(app->status), 0);
    gtk_box_append(GTK_BOX(main_box), app->status);

    refresh_files(app);
    gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char **argv)
{
    App app = {0};

    GtkApplication *application = gtk_application_new(
        "com.osmini.filesystem",
        G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(application, "activate",
                     G_CALLBACK(activate), &app);

    int status = g_application_run(
        G_APPLICATION(application), argc, argv);

    g_object_unref(application);
    g_free(app.current_path);

    return status;
}