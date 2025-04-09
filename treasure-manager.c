#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#define MAX_ID 16
#define MAX_NAME 32
#define MAX_USER 32
#define MAX_CLUE 128

typedef struct {
    char id[MAX_ID];         
    char name[MAX_NAME];     
    char username[MAX_USER];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

void add_treasure(const char* hunt_path) {
    char data_file[256], log_file[256], link_name[256];

    // Build paths
    snprintf(data_file, sizeof(data_file), "%s/treasure.dat", hunt_path);
    snprintf(log_file, sizeof(log_file), "%s/logged_hunt", hunt_path);
    snprintf(link_name, sizeof(link_name), "./logged_hunt-%s",
             strrchr(hunt_path, '/') ? strrchr(hunt_path, '/') + 1 : hunt_path);

    // ✅ Create the directory if it doesn't exist
    struct stat st;
    if (stat(hunt_path, &st) == -1) {
        if (mkdir(hunt_path, 0755) != 0) {
            perror("Could not create hunt directory");
            return;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' exists but is not a directory.\n", hunt_path);
        return;
    }

    // Open treasure.dat in append mode
    int fd = open(data_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening treasure.dat");
        return;
    }

    Treasure t;
    printf("Enter Treasure ID: ");
    scanf("%15s", t.id);

    printf("Enter Treasure Name: ");
    scanf("%31s", t.name);

    printf("Enter Username: ");
    scanf("%31s", t.username);

    printf("Enter Latitude: ");
    scanf("%f", &t.latitude);

    printf("Enter Longitude: ");
    scanf("%f", &t.longitude);

    printf("Enter Clue: ");
    getchar(); // Consume newline
    fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;

    printf("Enter Value: ");
    scanf("%d", &t.value);

    // Write the treasure
    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Write failed");
        close(fd);
        return;
    }
    close(fd);
    printf("Treasure added successfully.\n");

    // Append to log file
    int log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd >= 0) {
        dprintf(log_fd, "ADD: %s (%s) by %s\n", t.id, t.name, t.username);
        close(log_fd);
    }

    // Create symlink in root directory (if not already there)
    if (access(link_name, F_OK) == -1) {
        if (symlink(log_file, link_name) == -1) {
            perror("Symlink creation failed");
        }
    }
}


void list_treasures(const char* hunt_path) {
    char data_file[256];
    snprintf(data_file, sizeof(data_file), "%s/treasure.dat", hunt_path);

    struct stat file_stat;
    if (stat(data_file, &file_stat) == -1) {
        perror("Could not access treasure.dat");
        return;
    }

    const char* hunt_name = strrchr(hunt_path, '/');
    hunt_name = hunt_name ? hunt_name + 1 : hunt_path;

    printf("Hunt: %s\n", hunt_name);
    printf("File size: %ld bytes\n", file_stat.st_size);
    printf("Last modified: %s", ctime(&file_stat.st_mtime));

    int fd = open(data_file, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open treasure.dat");
        return;
    }

    Treasure t;
    printf("\nTreasure list:\n");
    printf("------------------------------------------------------------\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s\n", t.id);
        printf("Name: %s\n", t.name);
        printf("User: %s\n", t.username);
        printf("Location: (%f, %f)\n", t.latitude, t.longitude);
        printf("Clue: %s\n", t.clue);
        printf("Value: %d\n", t.value);
        printf("------------------------------------------------------------\n");
    }

    close(fd);
}

void view_treasure(const char* hunt_path, const char* treasure_id) {
    char data_file[256];
    snprintf(data_file, sizeof(data_file), "%s/treasure.dat", hunt_path);

    int fd = open(data_file, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open treasure.dat");
        return;
    }

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {
            printf("Treasure found:\n");
            printf("------------------------------------------------------------\n");
            printf("ID: %s\n", t.id);
            printf("Name: %s\n", t.name);
            printf("User: %s\n", t.username);
            printf("Location: (%f, %f)\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            printf("------------------------------------------------------------\n");
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) {
        printf("Treasure with ID '%s' not found.\n", treasure_id);
    }
}

void remove_treasure(const char* hunt_path, const char* treasure_id) {
    char data_file[256], temp_file[256], log_file[256];
    snprintf(data_file, sizeof(data_file), "%s/treasure.dat", hunt_path);
    snprintf(temp_file, sizeof(temp_file), "%s/temp.dat", hunt_path);
    snprintf(log_file, sizeof(log_file), "%s/logged_hunt", hunt_path);

    int fd = open(data_file, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open treasure.dat");
        return;
    }

    int temp_fd = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) {
        perror("Failed to open temp.dat");
        close(fd);
        return;
    }

    Treasure t;
    int index = 1;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {
            found = 1;
            continue;
        }

        snprintf(t.id, MAX_ID, "treasure%d", index++);
        write(temp_fd, &t, sizeof(Treasure));
    }

    close(fd);
    close(temp_fd);

    if (!found) {
        printf("Treasure with ID '%s' not found.\n", treasure_id);
        remove(temp_file);
        return;
    }

    if (remove(data_file) != 0 || rename(temp_file, data_file) != 0) {
        perror("Failed to update treasure.dat");
        return;
    }

    int log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd >= 0) {
        dprintf(log_fd, "REMOVE: %s\n", treasure_id);
        close(log_fd);
    }

    printf("Treasure '%s' removed and remaining IDs reindexed.\n", treasure_id);
}

void remove_hunt(const char* hunt_path) {
    char log_link[256];
    snprintf(log_link, sizeof(log_link), "./logged_hunt-%s",
             strrchr(hunt_path, '/') ? strrchr(hunt_path, '/') + 1 : hunt_path);

    DIR* dir = opendir(hunt_path);
    if (!dir) {
        perror("Hunt directory does not exist");
        return;
    }

    struct dirent* entry;
    char file_path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", hunt_path, entry->d_name);
        if (remove(file_path) != 0) {
            perror("Failed to remove a file inside hunt directory");
        }
    }
    closedir(dir);

    if (rmdir(hunt_path) != 0) {
        perror("Failed to remove hunt directory");
        return;
    }

    if (access(log_link, F_OK) == 0) {
        if (remove(log_link) != 0) {
            perror("Failed to remove symlink");
        }
    }

    printf("Hunt '%s' and all its files have been removed.\n", hunt_path);
}

int main(int argc, char*argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <command> <hunt_directory> [args...]\n", argv[0]);
        return 1;
    }

    const char* command = argv[1];
    const char* hunt_path = argv[2];

    if (strcmp(command, "add") == 0) {
        add_treasure(hunt_path);
    } else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_path);
    } else if (strcmp(command, "view") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s view <hunt_directory> <treasure_id>\n", argv[0]);
            return 1;
        }
        view_treasure(hunt_path, argv[3]);
    }else if (strcmp(command, "remove_treasure") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s remove_treasure <hunt_directory> <treasure_id>\n", argv[0]);
            return 1;
        }
        remove_treasure(hunt_path, argv[3]);
    }else if(strcmp(command, "remove_hunt")==0){
        remove_hunt(hunt_path);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}