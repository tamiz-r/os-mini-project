
#include <gtk/gtk.h>
#include <stdio.h>

/* Global widgets */
static GtkWidget *window;
static GtkWidget *file_list;
static GtkWidget *path_entry;
static GtkLabel *status_label;
static GtkWidget *location_label;

static char *current_path = NULL;

/* Update status message */
static void set_status(const char *message)
{
    gtk_label_set_text(status_label, message);
}

/* Clear file list */
static void clear_file_list(void)
{
    GtkWidget *child;

    while ((child = gtk_widget_get_first_child(file_list)) != NULL)
        gtk_list_box_remove(GTK_LIST_BOX(file_list), child);
}

/* Navigate to a directory */
static void navigate_to(const char *path);

/* Open a directory when a row is activated */
static void on_row_activated(GtkListBox *box,
                             GtkListBoxRow *row,
                             gpointer data)
{
    const char *path =
        g_object_get_data(G_OBJECT(row), "file-path");

    gboolean is_directory =
        GPOINTER_TO_INT(
            g_object_get_data(G_OBJECT(row), "is-directory"));

    if (is_directory && path != NULL) {
        navigate_to(path);
    } else {
        set_status("Selected file. File editing will be added next.");
    }
}

/* Add a file or directory row */
static void add_file_row(const char *name,
                         const char *path,
                         gboolean is_directory)
{
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    GtkWidget *icon = gtk_image_new_from_icon_name(
        is_directory ? "folder-symbolic"
                     : "text-x-generic-symbolic");

    GtkWidget *label = gtk_label_new(name);

    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);

    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_widget_set_hexpand(label, TRUE);

    gtk_box_append(GTK_BOX(box), icon);
    gtk_box_append(GTK_BOX(box), label);

    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);

    g_object_set_data_full(
        G_OBJECT(row), "file-path", g_strdup(path), g_free);

    g_object_set_data(
        G_OBJECT(row), "is-directory",
        GINT_TO_POINTER(is_directory));

    gtk_list_box_append(GTK_LIST_BOX(file_list), row);
}

/* Refresh the current directory */
static void refresh_directory(void)
{
    if (current_path == NULL)
        return;

    clear_file_list();

    GFile *directory = g_file_new_for_path(current_path);
    GError *error = NULL;

    GFileEnumerator *enumerator =
        g_file_enumerate_children(
            directory,
            "standard::name,standard::type",
            G_FILE_QUERY_INFO_NONE,
            NULL,
            &error);

    if (enumerator == NULL) {
        set_status(error->message);
        g_error_free(error);
        g_object_unref(directory);
        return;
    }

    int count = 0;
    GFileInfo *info;

    while ((info = g_file_enumerator_next_file(
                enumerator, NULL, &error)) != NULL) {

        const char *name = g_file_info_get_name(info);

        if (name != NULL) {
            char *child_path =
                g_build_filename(current_path, name, NULL);

            gboolean is_directory =
                g_file_info_get_file_type(info) ==
                G_FILE_TYPE_DIRECTORY;

            add_file_row(name, child_path, is_directory);

            g_free(child_path);
            count++;
        }

        g_object_unref(info);
    }

    if (error != NULL) {
        set_status(error->message);
        g_error_free(error);
    } else {
        char *message =
            g_strdup_printf("%d items", count);

        set_status(message);
        g_free(message);
    }

    g_object_unref(enumerator);
    g_object_unref(directory);
}

/* Navigate to a directory */
static void navigate_to(const char *path)
{
    if (path == NULL || path[0] == '\0')
        return;

    GFile *file = g_file_new_for_path(path);

    /* Correct directory validation */
    if (g_file_query_file_type(
            file, G_FILE_QUERY_INFO_NONE, NULL)
            != G_FILE_TYPE_DIRECTORY) {

        g_object_unref(file);
        set_status("Invalid directory.");
        return;
    }

    g_free(current_path);
    current_path = g_strdup(path);

    gtk_editable_set_text(
        GTK_EDITABLE(path_entry), current_path);

    gtk_label_set_text(
        GTK_LABEL(location_label), current_path);

    g_object_unref(file);

    refresh_directory();
}

/* Location entry */
static void on_path_enter(GtkEntry *entry, gpointer data)
{
    const char *path =
        gtk_editable_get_text(GTK_EDITABLE(entry));

    navigate_to(path);
}

/* Sidebar navigation */
static void on_sidebar_clicked(GtkButton *button, gpointer data)
{
    const char *path = data;
    navigate_to(path);
}

/* Go to parent directory */
static void on_up_clicked(GtkButton *button, gpointer data)
{
    if (current_path == NULL)
        return;

    char *parent = g_path_get_dirname(current_path);

    navigate_to(parent);

    g_free(parent);
}

/* Refresh button */
static void on_refresh_clicked(GtkButton *button, gpointer data)
{
    refresh_directory();
}

/* Create sidebar button */
static GtkWidget *create_sidebar_button(
    const char *label,
    const char *icon_name,
    const char *path)
{
    GtkWidget *button = gtk_button_new();
    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *icon =
        gtk_image_new_from_icon_name(icon_name);

    GtkWidget *text = gtk_label_new(label);

    gtk_label_set_xalign(GTK_LABEL(text), 0);
    gtk_widget_set_hexpand(text, TRUE);

    gtk_box_append(GTK_BOX(box), icon);
    gtk_box_append(GTK_BOX(box), text);

    gtk_button_set_child(GTK_BUTTON(button), box);

    g_signal_connect(
        button, "clicked",
        G_CALLBACK(on_sidebar_clicked),
        (gpointer)path);

    return button;
}

/* Main window */
static void activate(GtkApplication *app, gpointer user_data)
{
    window = gtk_application_window_new(app);

    gtk_window_set_title(
        GTK_WINDOW(window), "File System Interface");

    gtk_window_set_default_size(
        GTK_WINDOW(window), 1000, 650);

    /* Main layout */
    GtkWidget *main_box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_window_set_child(GTK_WINDOW(window), main_box);

    /* Toolbar */
    GtkWidget *toolbar =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    gtk_widget_set_margin_start(toolbar, 12);
    gtk_widget_set_margin_end(toolbar, 12);
    gtk_widget_set_margin_top(toolbar, 10);
    gtk_widget_set_margin_bottom(toolbar, 10);

    gtk_box_append(GTK_BOX(main_box), toolbar);

    GtkWidget *up_button =
        gtk_button_new_from_icon_name("go-up-symbolic");

    gtk_widget_set_tooltip_text(
        up_button, "Go to parent folder");

    g_signal_connect(
        up_button, "clicked",
        G_CALLBACK(on_up_clicked), NULL);

    gtk_box_append(GTK_BOX(toolbar), up_button);

    path_entry = gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(path_entry), "Enter directory path");

    gtk_widget_set_hexpand(path_entry, TRUE);

    g_signal_connect(
        path_entry, "activate",
        G_CALLBACK(on_path_enter), NULL);

    gtk_box_append(GTK_BOX(toolbar), path_entry);

    GtkWidget *refresh_button =
        gtk_button_new_from_icon_name("view-refresh-symbolic");

    gtk_widget_set_tooltip_text(
        refresh_button, "Refresh");

    g_signal_connect(
        refresh_button, "clicked",
        G_CALLBACK(on_refresh_clicked), NULL);

    gtk_box_append(GTK_BOX(toolbar), refresh_button);

    /* Main content area */
    GtkWidget *content =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_widget_set_vexpand(content, TRUE);

    gtk_box_append(GTK_BOX(main_box), content);

    /* Sidebar */
    GtkWidget *sidebar =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    gtk_widget_set_size_request(sidebar, 210, -1);
    gtk_widget_set_margin_start(sidebar, 12);
    gtk_widget_set_margin_end(sidebar, 12);
    gtk_widget_set_margin_top(sidebar, 12);

    gtk_box_append(GTK_BOX(content), sidebar);

    GtkWidget *places_label = gtk_label_new("Places");

    gtk_label_set_xalign(GTK_LABEL(places_label), 0);

    gtk_box_append(GTK_BOX(sidebar), places_label);

    const char *home = g_get_home_dir();

    char *documents =
        g_build_filename(home, "Documents", NULL);

    char *downloads =
        g_build_filename(home, "Downloads", NULL);

    gtk_box_append(
        GTK_BOX(sidebar),
        create_sidebar_button(
            "Home", "user-home-symbolic", home));

    gtk_box_append(
        GTK_BOX(sidebar),
        create_sidebar_button(
            "Documents", "folder-documents-symbolic",
            documents));

    gtk_box_append(
        GTK_BOX(sidebar),
        create_sidebar_button(
            "Downloads", "folder-download-symbolic",
            downloads));

    g_free(documents);
    g_free(downloads);

    /* Separator */
    GtkWidget *separator =
        gtk_separator_new(GTK_ORIENTATION_VERTICAL);

    gtk_box_append(GTK_BOX(content), separator);

    /* File browser */
    GtkWidget *browser =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    gtk_widget_set_hexpand(browser, TRUE);
    gtk_widget_set_vexpand(browser, TRUE);

    gtk_widget_set_margin_start(browser, 16);
    gtk_widget_set_margin_end(browser, 16);
    gtk_widget_set_margin_top(browser, 12);
    gtk_widget_set_margin_bottom(browser, 12);

    gtk_box_append(GTK_BOX(content), browser);

    location_label = gtk_label_new("Home");

    gtk_label_set_xalign(GTK_LABEL(location_label), 0);

    gtk_box_append(GTK_BOX(browser), location_label);

    GtkWidget *scroll = gtk_scrolled_window_new();

    gtk_widget_set_vexpand(scroll, TRUE);

    file_list = gtk_list_box_new();

    gtk_list_box_set_selection_mode(
        GTK_LIST_BOX(file_list), GTK_SELECTION_SINGLE);

    g_signal_connect(
        file_list, "row-activated",
        G_CALLBACK(on_row_activated), NULL);

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scroll), file_list);

    gtk_box_append(GTK_BOX(browser), scroll);

    /* Status bar */
    status_label =
        GTK_LABEL(gtk_label_new("Ready"));

    gtk_widget_set_margin_start(
        GTK_WIDGET(status_label), 12);

    gtk_widget_set_margin_top(
        GTK_WIDGET(status_label), 8);

    gtk_widget_set_margin_bottom(
        GTK_WIDGET(status_label), 8);

    gtk_label_set_xalign(status_label, 0);

    gtk_box_append(
        GTK_BOX(main_box), GTK_WIDGET(status_label));

    /* Start in home directory */
    navigate_to(home);

    gtk_window_present(GTK_WINDOW(window));
}

/* Main function */
int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "com.osproject.filesystem",
        G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(
        app, "activate",
        G_CALLBACK(activate), NULL);

    int status = g_application_run(
        G_APPLICATION(app), argc, argv);

    g_free(current_path);
    g_object_unref(app);

    return status;
}