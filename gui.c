
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

/* Global widgets */
static GtkLabel *status_label;
static GtkWidget *file_entry;
static GtkWidget *content_view;

/* Update status message */
static void set_status(const char *message)
{
    gtk_label_set_text(status_label, message);
}

/* Get filename or directory path */
static const char *get_path(void)
{
    return gtk_editable_get_text(GTK_EDITABLE(file_entry));
}

/* Get text from editor */
static char *get_editor_text(void)
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(content_view));

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

/* Replace editor content */
static void set_editor_text(const char *text)
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(content_view));

    gtk_text_buffer_set_text(buffer, text, -1);
}

/* CREATE FILE */
static void create_file(void)
{
    const char *path = get_path();

    if (path[0] == '\0') {
        set_status("Error: Enter a file name.");
        return;
    }

    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        set_status("Error: File already exists.");
        return;
    }

    GError *error = NULL;

    if (!g_file_set_contents(path, "", 0, &error)) {
        set_status(error->message);
        g_error_free(error);
        return;
    }

    set_status("Success: File created.");
}

/* READ FILE */
static void read_file(void)
{
    const char *path = get_path();

    if (path[0] == '\0') {
        set_status("Error: Enter a file name.");
        return;
    }

    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    if (!g_file_get_contents(path, &contents, &length, &error)) {
        set_status(error->message);
        g_error_free(error);
        return;
    }

    set_editor_text(contents);
    g_free(contents);

    set_status("Success: File read.");
}

/* WRITE FILE */
static void write_file(void)
{
    const char *path = get_path();

    if (path[0] == '\0') {
        set_status("Error: Enter a file name.");
        return;
    }

    char *content = get_editor_text();
    GError *error = NULL;

    if (!g_file_set_contents(path, content, -1, &error)) {
        set_status(error->message);
        g_error_free(error);
        g_free(content);
        return;
    }

    g_free(content);
    set_status("Success: File saved.");
}

/* DELETE FILE */
static void delete_file(void)
{
    const char *path = get_path();

    if (path[0] == '\0') {
        set_status("Error: Enter a file name.");
        return;
    }

    GError *error = NULL;

    if (!g_file_delete(g_file_new_for_path(path), NULL, &error)) {
        set_status(error->message);
        g_error_free(error);
        return;
    }

    set_status("Success: File deleted.");
}

/* CREATE DIRECTORY */
static void create_directory(void)
{
    const char *path = get_path();

    if (path[0] == '\0') {
        set_status("Error: Enter a directory name.");
        return;
    }

    GError *error = NULL;

    if (!g_file_make_directory(g_file_new_for_path(path),
                               NULL, &error)) {
        set_status(error->message);
        g_error_free(error);
        return;
    }

    set_status("Success: Directory created.");
}

/* LIST DIRECTORY */
static void list_directory(void)
{
    const char *path = get_path();

    if (path[0] == '\0') {
        set_status("Error: Enter a directory path.");
        return;
    }

    GFile *directory = g_file_new_for_path(path);
    GError *error = NULL;

    GFileEnumerator *enumerator =
        g_file_enumerate_children(
            directory,
            G_FILE_ATTRIBUTE_STANDARD_NAME,
            G_FILE_QUERY_INFO_NONE,
            NULL,
            &error
        );

    if (enumerator == NULL) {
        set_status(error->message);
        g_error_free(error);
        g_object_unref(directory);
        return;
    }

    GString *listing = g_string_new("");

    GFileInfo *info;

    while ((info = g_file_enumerator_next_file(
                enumerator, NULL, &error)) != NULL) {

        const char *name = g_file_info_get_name(info);
        g_string_append_printf(listing, "%s\n", name);
        g_object_unref(info);
    }

    g_object_unref(enumerator);
    g_object_unref(directory);

    if (error != NULL) {
        set_status(error->message);
        g_error_free(error);
        g_string_free(listing, TRUE);
        return;
    }

    set_editor_text(listing->str);
    g_string_free(listing, TRUE);

    set_status("Success: Directory listed.");
}

/* Button callback */
static void on_operation_clicked(GtkButton *button,
                                 gpointer user_data)
{
    const char *operation = user_data;

    if (g_strcmp0(operation, "Create") == 0)
        create_file();

    else if (g_strcmp0(operation, "Read") == 0)
        read_file();

    else if (g_strcmp0(operation, "Write") == 0)
        write_file();

    else if (g_strcmp0(operation, "Delete") == 0)
        delete_file();

    else if (g_strcmp0(operation, "Create Directory") == 0)
        create_directory();

    else if (g_strcmp0(operation, "List Directory") == 0)
        list_directory();
}

/* Create button */
static GtkWidget *create_button(const char *label,
                                const char *operation)
{
    GtkWidget *button = gtk_button_new_with_label(label);

    g_signal_connect(button, "clicked",
                     G_CALLBACK(on_operation_clicked),
                     (gpointer)operation);

    return button;
}

/* Main window */
static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *button_box;
    GtkWidget *dir_box;
    GtkWidget *content_label;
    GtkWidget *name_label;
    GtkWidget *scroll;

    window = gtk_application_window_new(app);

    gtk_window_set_title(GTK_WINDOW(window),
                         "File System Interface");

    gtk_window_set_default_size(GTK_WINDOW(window), 800, 550);

    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    gtk_widget_set_margin_start(main_box, 20);
    gtk_widget_set_margin_end(main_box, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);

    gtk_window_set_child(GTK_WINDOW(window), main_box);

    /* Filename input */
    name_label = gtk_label_new("File Name / Directory Path:");
    gtk_label_set_xalign(GTK_LABEL(name_label), 0);
    gtk_box_append(GTK_BOX(main_box), name_label);

    file_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(
        GTK_ENTRY(file_entry), "Enter file name or directory path..."
    );

    gtk_box_append(GTK_BOX(main_box), file_entry);

    /* File operations */
    GtkWidget *file_label = gtk_label_new("File Operations");
    gtk_label_set_xalign(GTK_LABEL(file_label), 0);
    gtk_box_append(GTK_BOX(main_box), file_label);

    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(main_box), button_box);

    gtk_box_append(GTK_BOX(button_box),
                   create_button("Create", "Create"));

    gtk_box_append(GTK_BOX(button_box),
                   create_button("Read", "Read"));

    gtk_box_append(GTK_BOX(button_box),
                   create_button("Write", "Write"));

    gtk_box_append(GTK_BOX(button_box),
                   create_button("Delete", "Delete"));

    /* Directory operations */
    dir_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(main_box), dir_box);

    gtk_box_append(GTK_BOX(dir_box),
                   create_button("Create Directory",
                                 "Create Directory"));

    gtk_box_append(GTK_BOX(dir_box),
                   create_button("List Directory",
                                 "List Directory"));

    /* Content editor */
    content_label = gtk_label_new("File Content:");
    gtk_label_set_xalign(GTK_LABEL(content_label), 0);
    gtk_box_append(GTK_BOX(main_box), content_label);

    content_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(content_view),
                                GTK_WRAP_WORD_CHAR);

    scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scroll), content_view);

    gtk_box_append(GTK_BOX(main_box), scroll);

    /* Status */
    status_label = GTK_LABEL(gtk_label_new("Status: Ready"));
    gtk_label_set_xalign(status_label, 0);

    gtk_box_append(GTK_BOX(main_box),
                   GTK_WIDGET(status_label));

    gtk_window_present(GTK_WINDOW(window));
}

/* Main function */
int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new(
        "com.osproject.filesystem",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(app, "activate",
                     G_CALLBACK(activate), NULL);

    status = g_application_run(
        G_APPLICATION(app), argc, argv
    );

    g_object_unref(app);

    return status;
}