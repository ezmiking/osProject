// A C program to create a separate process for each store and process data files.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>

// Function to handle operations for a store
void handle_store(const char *store_path, const char *store_name) {
    printf("Process for %s started.\n", store_name);

    DIR *dir = opendir(store_path);
    if (dir == NULL) {
        perror("Failed to open directory");
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the file path
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/%s", store_path, entry->d_name);

        // Open the file and read its contents
        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
            perror("Failed to open file");
            continue;
        }

        printf("Processing file: %s\n", file_path);

        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) {
            printf("%s", line); // Print each line (or process as needed)
        }

        fclose(file);
    }

    closedir(dir);
    printf("Process for %s completed.\n", store_name);
}

int main() {
    // Fixed dataset path
    const char *dataset_path = "./Dataset";

    // List of store names
    const char *stores[] = {"Store1", "Store2", "Store3"};
    int num_stores = sizeof(stores) / sizeof(stores[0]);

    // Create processes for each store
    for (int i = 0; i < num_stores; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child process: construct the store path and handle the store
            char store_path[256];
            snprintf(store_path, sizeof(store_path), "%s/%s", dataset_path, stores[i]);
            handle_store(store_path, stores[i]);
            exit(0); // Exit child process after completing work
        } else if (pid > 0) {
            // Parent process: continue to create other processes
            printf("Parent: Created process for %s.\n", stores[i]);
        } else {
            // Fork failed
            perror("Fork failed");
            exit(1);
        }
    }

    // Parent process waits for all child processes to complete
    for (int i = 0; i < num_stores; i++) {
        wait(NULL);
    }

    printf("All store processes completed.\n");
    return 0;
}
