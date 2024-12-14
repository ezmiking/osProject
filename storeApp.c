// A C program to create a separate process for userID, each store, process categories within them, and create threads for each product.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>

// Shared user input data
char items[256][256];
int quantities[256];
int num_items = 0;

// Thread function to handle a product
void *handle_product(void *file_path) {
    char *path = (char *)file_path;

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("PID %d create thread TID:%ld for file %s - File not found\n", getpid(), syscall(SYS_gettid), path);
        pthread_exit(NULL);
    }

    printf("PID %d create thread TID:%ld for file %s\n", getpid(), syscall(SYS_gettid), path);

    char line[256];
    int found = 0;

    // Check if any item matches
    while (fgets(line, sizeof(line), file) != NULL) {
        for (int i = 0; i < num_items; i++) {
            if (strstr(line, items[i]) != NULL) {
                printf("Thread: Found '%s' in file %s\n", items[i], path);
                found = 1;
            }
        }
    }

    fclose(file);

    pthread_exit(NULL);
}

// Function to handle operations for a category
void handle_category(const char *category_path, const char *category_name) {
    printf("PID %d create child for category %s\n", getpid(), category_name);

    DIR *dir = opendir(category_path);
    if (dir == NULL) {
        perror("Failed to open directory");
        exit(1);
    }

    struct dirent *entry;
    pthread_t threads[256];
    int thread_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the file path
        char *file_path = malloc(256);
        snprintf(file_path, 256, "%s/%s", category_path, entry->d_name);

        // Create a thread for each product file
        if (pthread_create(&threads[thread_count], NULL, handle_product, file_path) != 0) {
            perror("Failed to create thread");
            free(file_path);
            continue;
        }

        thread_count++;
    }

    // Wait for all threads to complete
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    closedir(dir);
}

// Function to handle operations for a store
void handle_store(const char *store_path, const char *store_name) {
    printf("PID %d create child for store %s\n", getpid(), store_name);

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

        // Construct the category path
        char category_path[256];
        snprintf(category_path, sizeof(category_path), "%s/%s", store_path, entry->d_name);

        // Create a process for each category
        pid_t pid = fork();

        if (pid == 0) {
            // Child process: handle the category
            handle_category(category_path, entry->d_name);
            exit(0); // Exit child process after completing work
        } else if (pid > 0) {
            // Parent process: continue to process other categories
            printf("PID %d create child for category %s PID:%d\n", getpid(), entry->d_name, pid);
        } else {
            // Fork failed
            perror("Fork failed");
            exit(1);
        }
    }

    // Parent process waits for all category processes to complete
    while (wait(NULL) > 0);

    closedir(dir);
}

int main() {
    // Fixed dataset path
    const char *dataset_path = "./Dataset";

    // List of store names
    const char *stores[] = {"Store1", "Store2", "Store3"};
    int num_stores = sizeof(stores) / sizeof(stores[0]);

    // Create a process for userID
    pid_t user_pid = fork();

    if (user_pid == 0) {
        // Child process: userID process
        printf("USER1 create PID: %d\n", getpid());

        // Get user name
        char user_name[256];
        printf("Username: ");
        scanf("%s", user_name);
        printf("\nOrderList0:\n");

        // Get shopping list from the user
        num_items = 0;
        while (1) {
            printf("Enter the name of item %d: ", num_items + 1);
            scanf("%s", items[num_items]);
            if (strcmp(items[num_items], "done") == 0) {
                break;
            }
            printf("Enter the quantity for %s: ", items[num_items]);
            scanf("%d", &quantities[num_items]);
            num_items++;
        }

        // Clear input buffer
        while (getchar() != '\n');

        printf("Price threshold: ");
        char budget_input[256];
        fgets(budget_input, sizeof(budget_input), stdin);

        printf("\nUsername: %s\nOrderList0:\n", user_name);
        for (int i = 0; i < num_items; i++) {
            printf("%s %d\n", items[i], quantities[i]);
        }

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
                printf("PID %d create child for store %s PID:%d\n", getpid(), stores[i], pid);
            } else {
                // Fork failed
                perror("Fork failed");
                exit(1);
            }
        }

        // Wait for all store processes to complete
        for (int i = 0; i < num_stores; i++) {
            wait(NULL);
        }

        printf("Process for userID completed.\n");
        exit(0);
    } else if (user_pid > 0) {
        // Parent process: wait for userID process to complete
        wait(NULL);
    } else {
        perror("Fork failed for userID process");
        exit(1);
    }

    printf("All processes completed successfully.\n");
    return 0;
}
