// A C program to create a separate process for userID, each store, process categories within them, and create threads for each product.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>

// Thread function to handle a product
void *handle_product(void *file_path) {
    char *path = (char *)file_path;
    printf("Thread: Processing file %s\n", path);

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("Failed to open file");
        pthread_exit(NULL);
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }

    fclose(file);
    printf("Thread: Completed processing file %s\n", path);
    pthread_exit(NULL);
}

// Function to handle operations for a category
void handle_category(const char *category_path, const char *category_name) {
    printf("Process for category %s started.\n", category_name);

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
    printf("Process for category %s completed.\n", category_name);
}

// Function to handle operations for a store
void handle_store(const char *store_path, const char *store_name) {
    printf("Process for store %s started.\n", store_name);

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
            printf("Parent: Created process for category %s in store %s.\n", entry->d_name, store_name);
        } else {
            // Fork failed
            perror("Fork failed");
            exit(1);
        }
    }

    // Parent process waits for all category processes to complete
    while (wait(NULL) > 0);

    closedir(dir);
    printf("Process for store %s completed.\n", store_name);
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
        printf("Process for userID started.\n");

        // Get user name
        char user_name[256];
        printf("Username: ");
        scanf("%s", user_name);
        printf("\nOrderList0:\n");

        // Get shopping list from the user
        char items[256][256];
        int quantities[256];
        int num_items = 0;
        double budget;

        while (1) {
            printf("Enter the name of the item (or type 'done' to finish): ");
            scanf("%s", items[num_items]);
            if (strcmp(items[num_items], "done") == 0) {
                break;
            }
            printf("%s ", items[num_items]);
            scanf("%d", &quantities[num_items]);
            printf("\n");
            num_items++;
        }

        printf("Price threshold: ");
        char budget_input[256];
        scanf("%s", budget_input);

        if (strlen(budget_input) == 0 || strcmp(budget_input, "none") == 0) {
            printf("No price threshold defined.\n");
            budget = -1;
        } else {
            budget = atof(budget_input);
        }

        printf("\nUsername: %s\nOrderList0:\n", user_name);
        for (int i = 0; i < num_items; i++) {
            printf("%s %d\n", items[i], quantities[i]);
        }
        if (budget > 0) {
            printf("Price threshold: %.2lf\n", budget);
        } else {
            printf("Price threshold: none\n");
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
                printf("UserID Process: Created process for store %s.\n", stores[i]);
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
