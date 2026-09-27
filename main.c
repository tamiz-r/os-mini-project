
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define MAKE_DIR(path) _mkdir(path)
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #define MAKE_DIR(path) mkdir(path, 0755)
#endif

#define MAX 512

// ---------- Utility ----------
void trimNewline(char *s) {
    s[strcspn(s, "\r\n")] = '\0';
}

void getInput(const char *message, char *buffer, size_t size) {
    printf("%s", message);
    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    trimNewline(buffer);
}

// ---------- Create File ----------
void createFile() {
    char name[MAX];
    getInput("Enter filename: ", name, sizeof(name));

    FILE *fp = fopen(name, "wx");
    if (!fp) {
        perror("Cannot create file");
        return;
    }

    fclose(fp);
    printf("File created successfully.\n");
}

// ---------- Read File ----------
void readFile() {
    char name[MAX], buffer[MAX];
    getInput("Enter filename: ", name, sizeof(name));

    FILE *fp = fopen(name, "r");
    if (!fp) {
        perror("Cannot open file");
        return;
    }

    printf("\n--- File Contents ---\n");
    while (fgets(buffer, sizeof(buffer), fp))
        printf("%s", buffer);

    fclose(fp);
    printf("\n---------------------\n");
}

// ---------- Write File ----------
void writeFile() {
    char name[MAX], content[MAX];
    getInput("Enter filename: ", name, sizeof(name));

    FILE *fp = fopen(name, "w");
    if (!fp) {
        perror("Cannot open file");
        return;
    }

    getInput("Enter content: ", content, sizeof(content));

    if (fprintf(fp, "%s\n", content) < 0)
        perror("Write failed");
    else
        printf("Content written successfully.\n");

    fclose(fp);
}

// ---------- Delete File ----------
void deleteFile() {
    char name[MAX];
    getInput("Enter filename to delete: ", name, sizeof(name));

    if (remove(name) == 0)
        printf("File deleted successfully.\n");
    else
        perror("Delete failed");
}

// ---------- Create Directory ----------
void createDirectory() {
    char name[MAX];
    getInput("Enter directory name: ", name, sizeof(name));

    if (MAKE_DIR(name) == 0)
        printf("Directory created successfully.\n");
    else
        perror("Directory creation failed");
}

// ---------- List Directory ----------
void listFiles() {
    char path[MAX];
    getInput("Enter directory path (use . for current): ",
             path, sizeof(path));

#ifdef _WIN32
    char pattern[MAX];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);

    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA(pattern, &data);

    if (h == INVALID_HANDLE_VALUE) {
        printf("Cannot open directory.\n");
        return;
    }

    printf("\nDirectory contents:\n");
    do {
        printf("%s\n", data.cFileName);
    } while (FindNextFileA(h, &data));

    FindClose(h);

#else
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Cannot open directory");
        return;
    }

    struct dirent *entry;
    printf("\nDirectory contents:\n");

    while ((entry = readdir(dir)) != NULL)
        printf("%s\n", entry->d_name);

    closedir(dir);
#endif
}

// ---------- Sequential Access ----------
void sequentialAccess() {
    char name[MAX], buffer[MAX];
    getInput("Enter filename: ", name, sizeof(name));

    FILE *fp = fopen(name, "rb");
    if (!fp) {
        perror("Cannot open file");
        return;
    }

    printf("\nSequential reading:\n");

    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        fwrite(buffer, 1, n, stdout);

    fclose(fp);
    printf("\n");
}

// ---------- Direct Access ----------
void directAccess() {
    char name[MAX], input[64];
    long offset;

    getInput("Enter filename: ", name, sizeof(name));
    getInput("Enter byte offset: ", input, sizeof(input));

    char *end;
    errno = 0;
    offset = strtol(input, &end, 10);

    while (isspace((unsigned char)*end))
        end++;

    if (errno || end == input || *end != '\0' || offset < 0) {
        printf("Invalid offset.\n");
        return;
    }

    FILE *fp = fopen(name, "rb");
    if (!fp) {
        perror("Cannot open file");
        return;
    }

    if (fseek(fp, offset, SEEK_SET) != 0) {
        perror("Seek failed");
        fclose(fp);
        return;
    }

    char buffer[MAX];
    size_t n = fread(buffer, 1, sizeof(buffer), fp);

    printf("\nData from byte offset %ld:\n", offset);
    if (n > 0)
        fwrite(buffer, 1, n, stdout);
    else if (ferror(fp))
        perror("Read failed");
    else
        printf("End of file reached.\n");

    printf("\n");
    fclose(fp);
}

// ---------- Main Menu ----------
int main() {
    char input[64];
    int choice;

    while (1) {
        printf("\n================================\n");
        printf("    OS FILE SYSTEM INTERFACE\n");
        printf("================================\n");
        printf("1. Create File\n");
        printf("2. Read File\n");
        printf("3. Write File\n");
        printf("4. Delete File\n");
        printf("5. Create Directory\n");
        printf("6. List Directory\n");
        printf("7. Sequential Access\n");
        printf("8. Direct Access\n");
        printf("0. Exit\n");
        printf("--------------------------------\n");

        getInput("Enter choice: ", input, sizeof(input));

        char *end;
        long value = strtol(input, &end, 10);
        while (isspace((unsigned char)*end))
            end++;

        if (end == input || *end != '\0' ||
            value < 0 || value > 8) {
            printf("Invalid choice.\n");
            continue;
        }

        choice = (int)value;

        switch (choice) {
            case 1: createFile(); break;
            case 2: readFile(); break;
            case 3: writeFile(); break;
            case 4: deleteFile(); break;
            case 5: createDirectory(); break;
            case 6: listFiles(); break;
            case 7: sequentialAccess(); break;
            case 8: directAccess(); break;
            case 0:
                printf("Exiting program.\n");
                return 0;
        }
    }
}