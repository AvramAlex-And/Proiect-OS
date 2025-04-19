#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#define MAX_MONITORS 10

typedef struct {
    pid_t pid;
    char hunt_id[256];
} Monitor;

Monitor monitors[MAX_MONITORS];
int monitor_count = 0;

void kill_all_monitors() {
    for (int i = 0; i < monitor_count; i++) {
        kill(monitors[i].pid, SIGTERM);
    }
    sleep(1);
}

void monitor_hunt(const char* hunt_id) {
    char dat_path[512], log_path[512];
    struct stat dat_stat, log_stat, prev_dat_stat, prev_log_stat;

    snprintf(dat_path, sizeof(dat_path), "%s/treasure.dat", hunt_id);
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);

    stat(dat_path, &prev_dat_stat);
    stat(log_path, &prev_log_stat);

    printf("[Monitor %d] Monitoring hunt: %s\n", getpid(), hunt_id);

    while (1) {
        sleep(2);

        stat(dat_path, &dat_stat);
        stat(log_path, &log_stat);

        if (dat_stat.st_mtime != prev_dat_stat.st_mtime) {
            printf("[Monitor %d] treasure.dat in '%s' was modified.\n", getpid(), hunt_id);
            prev_dat_stat = dat_stat;
        }

        if (log_stat.st_mtime != prev_log_stat.st_mtime) {
            printf("[Monitor %d] logged_hunt in '%s' was modified.\n", getpid(), hunt_id);
            prev_log_stat = log_stat;
        }
    }
}

int main() {
    char command[256];

    while (1) {
        printf("treasure_hub> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0; // Trim newline

        if (strncmp(command, "start_monitor", 14) == 0) {
            if (monitor_count >= MAX_MONITORS) {
                printf("Max monitor limit reached.\n");
                continue;
            }
            char* hunt_id = command + 14;
            pid_t pid = fork();
            if (pid == 0) {
                monitor_hunt(hunt_id);
                exit(0);
            } else if (pid > 0) {
                monitors[monitor_count].pid = pid;
                strncpy(monitors[monitor_count].hunt_id, hunt_id, sizeof(monitors[monitor_count].hunt_id));
                monitor_count++;
                printf("Started monitor for '%s' (PID %d)\n", hunt_id, pid);
            } else {
                perror("fork failed");
            }
        } else if (strncmp(command, "add_treasure ", 13) == 0) {
            char* hunt_id = command + 13;
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "./treasure_manager add %s", hunt_id);
            system(cmd);
        } else if (strncmp(command, "view_treasure ", 14) == 0) {
            char hunt_id[256], treasure_id[256];
            if (sscanf(command + 14, "%s %s", hunt_id, treasure_id) == 2) {
                char cmd[512];
                snprintf(cmd, sizeof(cmd), "./treasure_manager view %s %s", hunt_id, treasure_id);
                system(cmd);
            } else {
                printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
            }
        } else if (strcmp(command, "list_hunts") == 0) {
            system("ls -d */ | grep -E '^hunt'");
        } else if (strcmp(command, "exit") == 0) {
            printf("Shutting down monitors...\n");
            kill_all_monitors();
            break;
        } else {
            printf("Unknown command: %s\n", command);
        }
    }

    return 0;
}