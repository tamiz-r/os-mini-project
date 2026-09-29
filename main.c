
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define make_dir(path) _mkdir(path)
#define change_dir(path) _chdir(path)
#define get_dir(path, size) _getcwd(path, size)
#define remove_dir(path) _rmdir(path)
#else
#include <unistd.h>
#define make_dir(path) mkdir(path, 0777)
#define change_dir(path) chdir(path)
#define get_dir(path, size) getcwd(path, size)
#define remove_dir(path) rmdir(path)
#endif

#define SIZE 1024

char current_path[SIZE];

/* Remove newline from input */
void trim_newline(char *str) {
    str[strcspn(str, "\n")] = '\0';
}

/* Get string input */
void get_input(const char *message, char *buffer, size_t size) {
    printf("%s", message);
    if (fgets(buffer, size, stdin) != NULL) {
        trim_newline(buffer);
    } else {
        buffer[0] = '\0';
    }
}

/* List files and directories */
void list_files(void) {
    DIR *dir = opendir(".");
    struct dirent *entry;

    if (dir == NULL) {
        perror("Cannot open directory");
        return;
    }

    printf("\n--- Files and Directories ---\n");

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

/* Create a new file */
void create_file(void) {
    char name[SIZE];
    get_input("Enter file name: ", name, sizeof(name));

    FILE *file = fopen(name, "wx");

    if (file == NULL) {
        perror("File creation failed");
        return;
    }

    fclose(file);
    printf("File created successfully.\n");
}

/* Read file sequentially */
void read_file(void) {
    char name[SIZE];
    char buffer[SIZE];

    get_input("Enter file name: ", name, sizeof(name));

    FILE *file = fopen(name, "r");

    if (file == NULL) {
        perror("File open failed");
        return;
    }

    printf("\n--- File Contents ---\n");

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    printf("\n");
    fclose(file);
}

/* Write or overwrite a file */
void write_file(void) {
    char name[SIZE];
    char content[SIZE];

    get_input("Enter file name: ", name, sizeof(name));
    get_input("Enter content: ", content, sizeof(content));

    FILE *file = fopen(name, "w");

    if (file == NULL) {
        perror("File write failed");
        return;
    }

    fprintf(file, "%s\n", content);
    fclose(file);

    printf("File written successfully.\n");
}

/* Append content to a file */
void append_file(void) {
    char name[SIZE];
    char content[SIZE];

    get_input("Enter file name: ", name, sizeof(name));
    get_input("Enter content to append: ", content, sizeof(content));

    FILE *file = fopen(name, "a");

    if (file == NULL) {
        perror("File open failed");
        return;
    }

    fprintf(file, "%s\n", content);
    fclose(file);

    printf("Content appended successfully.\n");
}

/* Delete a file */
void delete_file(void) {
    char name[SIZE];

    get_input("Enter file name: ", name, sizeof(name));

    if (remove(name) == 0)
        printf("File deleted successfully.\n");
    else
        perror("File deletion failed");
}

/* Create a directory */
void create_directory(void) {
    char name[SIZE];

    get_input("Enter directory name: ", name, sizeof(name));

    if (make_dir(name) == 0)
        printf("Directory created successfully.\n");
    else
        perror("Directory creation failed");
}

/* Rename a file or directory */
void rename_item(void) {
    char old_name[SIZE];
    char new_name[SIZE];

    get_input("Enter current name: ", old_name, sizeof(old_name));
    get_input("Enter new name: ", new_name, sizeof(new_name));

    if (rename(old_name, new_name) == 0)
        printf("Renamed successfully.\n");
    else
        perror("Rename failed");
}

/* Remove an empty directory */
void delete_directory(void) {
    char name[SIZE];

    get_input("Enter directory name: ", name, sizeof(name));

    if (remove_dir(name) == 0)
        printf("Directory deleted successfully.\n");
    else
        perror("Directory deletion failed (it may not be empty)");
}

/* Change current directory */
void navigate_directory(void) {
    char path[SIZE];

    get_input("Enter directory path (.. for parent): ",
              path, sizeof(path));

    if (change_dir(path) == 0) {
        if (get_dir(current_path, sizeof(current_path)) != NULL)
            printf("Current directory: %s\n", current_path);
    } else {
        perror("Directory change failed");
    }
}

/* Direct access: read bytes from a specified offset */
void direct_read(void) {
    char name[SIZE];
    long offset;
    int count;

    get_input("Enter file name: ", name, sizeof(name));

    printf("Enter byte offset: ");
    if (scanf("%ld", &offset) != 1) {
        printf("Invalid offset.\n");
        while (getchar() != '\n');
        return;
    }

    printf("Enter number of bytes to read: ");
    if (scanf("%d", &count) != 1 || count <= 0 || count > 4096) {
        printf("Invalid byte count (1-4096).\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    FILE *file = fopen(name, "rb");

    if (file == NULL) {
        perror("File open failed");
        return;
    }

    if (fseek(file, offset, SEEK_SET) != 0) {
        perror("Seek failed");
        fclose(file);
        return;
    }

    unsigned char *buffer = malloc((size_t)count);
    if (buffer == NULL) {
        printf("Memory allocation failed.\n");
        fclose(file);
        return;
    }

    size_t bytes = fread(buffer, 1, (size_t)count, file);

    printf("\n--- Data at offset %ld ---\n", offset);
    for (size_t i = 0; i < bytes; i++)
        printf("%02X ", buffer[i]);

    printf("\nBytes read: %zu\n", bytes);

    free(buffer);
    fclose(file);
}

/* Direct access: write bytes at a specified offset */
void direct_write(void) {
    char name[SIZE];
    char data[SIZE];
    long offset;

    get_input("Enter file name: ", name, sizeof(name));

    printf("Enter byte offset: ");
    if (scanf("%ld", &offset) != 1 || offset < 0) {
        printf("Invalid offset.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    get_input("Enter text to write: ", data, sizeof(data));

    FILE *file = fopen(name, "r+b");

    if (file == NULL) {
        perror("File open failed (file must exist)");
        return;
    }

    if (fseek(file, offset, SEEK_SET) != 0) {
        perror("Seek failed");
        fclose(file);
        return;
    }

    size_t len = strlen(data);
    size_t written = fwrite(data, 1, len, file);

    fclose(file);

    if (written == len)
        printf("Data written at offset %ld.\n", offset);
    else
        printf("Write was incomplete.\n");
}

/* Display menu */
void display_menu(void) {
    printf("\n========== FILE SYSTEM INTERFACE ==========\n");
    printf("Current Directory: %s\n", current_path);
    printf("-------------------------------------------\n");
    printf("1.  List Files and Directories\n");
    printf("2.  Create File\n");
    printf("3.  Read File (Sequential Access)\n");
    printf("4.  Write File (Overwrite)\n");
    printf("5.  Append to File\n");
    printf("6.  Delete File\n");
    printf("7.  Create Directory\n");
    printf("8.  Rename File or Directory\n");
    printf("9.  Delete Empty Directory\n");
    printf("10. Change Directory\n");
    printf("11. Direct Read (Byte Offset)\n");
    printf("12. Direct Write (Byte Offset)\n");
    printf("0.  Exit\n");
    printf("===========================================\n");
}

int main(void) {
    int choice;

    if (get_dir(current_path, sizeof(current_path)) == NULL) {
        strcpy(current_path, ".");
    }

    printf("Welcome to the File System Interface!\n");

    while (1) {
        display_menu();

        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        switch (choice) {
            case 1:  list_files();       break;
            case 2:  create_file();      break;
            case 3:  read_file();        break;
            case 4:  write_file();       break;
            case 5:  append_file();      break;
            case 6:  delete_file();      break;
            case 7:  create_directory(); break;
            case 8:  rename_item();      break;
            case 9:  delete_directory(); break;
            case 10: navigate_directory(); break;
            case 11: direct_read();      break;
            case 12: direct_write();     break;
            case 0:
                printf("Exiting File System Interface.\n");
                return 0;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}