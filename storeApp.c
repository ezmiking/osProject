// A C program to create a separate process for userID, each store, process categories within them, and create threads for each product.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>

// Shared user input data
char items[256][256];
int quantities[256];
int num_items = 0;

// Bag shop for each store
pthread_mutex_t cart_lock = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    char name[256][256];
    float price[100];
    float score[100];
    int entity[100];
    int name_count;
} cart_shop;

int shm_id; // Global shared memory identifier

// Thread function to handle a product
void *handle_product(void *args) {
    struct {
        char *file_path;
        cart_shop *store_cart;
    } *thread_args = args;

    char *path = thread_args->file_path;
    cart_shop *store_cart = thread_args->store_cart;

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("PID %d create thread TID:%ld for file %s - File not found\n", getpid(), syscall(SYS_gettid), path);
        free(thread_args);
        pthread_exit(NULL);
    }

    printf("PID %d create thread TID:%ld for file %s\n", getpid(), syscall(SYS_gettid), path);

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        char temp_name[256] = "";
        float temp_price = -1.0, temp_score = -1.0;
        int temp_entity = -1;

        if (sscanf(line, " Name: %255[^\n]", temp_name) == 1) {
        for (int i = 0; i < num_items; i++) {
            if (strcmp(temp_name, items[i]) == 0) {
                while (fgets(line, sizeof(line), file) != NULL) {
                    if (sscanf(line, " Price: %f", &temp_price) == 1) continue;
                    if (sscanf(line, " Score: %f", &temp_score) == 1) continue;
                    if (sscanf(line, " Entity: %d", &temp_entity) == 1) break;
                }

                if (temp_price > 0 && temp_score > 0 && temp_entity > 0) {
                    pthread_mutex_lock(&cart_lock);
                    strcpy(store_cart->name[store_cart->name_count], temp_name);
                    store_cart->price[store_cart->name_count] = temp_price;
                    store_cart->score[store_cart->name_count] = temp_score;
                    store_cart->entity[store_cart->name_count] = temp_entity;
                    store_cart->name_count++;
                    pthread_mutex_unlock(&cart_lock);

                    printf("Thread TID:%ld found product: %s, Price: %.2f, Score: %.2f, Entity: %d, Path: %s\n",
                           syscall(SYS_gettid), temp_name, temp_price, temp_score, temp_entity, path);
                }
                break;
            }
        }
    }
}

fclose(file);
free(thread_args);
pthread_exit(NULL);
}

// Function to handle operations for a category
void handle_category(const char *category_path, const char *category_name, cart_shop *store_cart) {
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
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *file_path = malloc(256);
        snprintf(file_path, 256, "%s/%s", category_path, entry->d_name);

        struct {
            char *file_path;
            cart_shop *store_cart;
        } *thread_args = malloc(sizeof(*thread_args));
        thread_args->file_path = file_path;
        thread_args->store_cart = store_cart;

        if (pthread_create(&threads[thread_count], NULL, handle_product, thread_args) != 0) {
            perror("Failed to create thread");
            free(thread_args);
            continue;
        }

        thread_count++;
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    closedir(dir);
}

// Function to handle operations for a store
void handle_store(const char *store_path, const char *store_name) {
    shm_id = shmget(IPC_PRIVATE, sizeof(cart_shop), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Failed to create shared memory");
        exit(1);
    }
    cart_shop *store_cart = (cart_shop *)shmat(shm_id, NULL, 0);
    if (store_cart == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }
    memset(store_cart, 0, sizeof(cart_shop));
    printf("PID %d create child for store %s\n", getpid(), store_name);

    DIR *dir = opendir(store_path);
    if (dir == NULL) {
        perror("Failed to open directory");
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char category_path[256];
        snprintf(category_path, sizeof(category_path), "%s/%s", store_path, entry->d_name);

        pid_t pid = fork();

        if (pid == 0) {
            handle_category(category_path, entry->d_name, store_cart);
            exit(0);
        } else if (pid > 0) {
            printf("PID %d create child for category %s PID:%d\n", getpid(), entry->d_name, pid);
        } else {
            perror("Fork failed");
            exit(1);
        }
    }

    while (wait(NULL) > 0);

    closedir(dir);

    printf("\nSummary of cart for store %s:\n", store_name);
    if (store_cart->name_count == 0) {
        printf("No items were added to the cart for store %s.\n", store_name);
    }
    for (int i = 0; i < store_cart->name_count; i++) {
        printf("Product: %s, Price: %.2f, Score: %.2f, Entity: %d\n",
               store_cart->name[i], store_cart->price[i], store_cart->score[i], store_cart->entity[i]);
    }

    if (shmdt(store_cart) == -1) {
        perror("Failed to detach shared memory");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Failed to remove shared memory");
    }
}

int main() {
    const char *dataset_path = "./Dataset";

    const char *stores[] = {"Store1", "Store2", "Store3"};
    int num_stores = sizeof(stores) / sizeof(stores[0]);

    pid_t user_pid = fork();

    if (user_pid == 0) {
        printf("USER1 create PID: %d\n", getpid());

        char user_name[256];
        printf("Username: ");
        scanf("%s", user_name);
        printf("\nOrderList0:\n");

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

        while (getchar() != '\n');

        printf("Price threshold: ");
        char budget_input[256];
        fgets(budget_input, sizeof(budget_input), stdin);

        printf("\nUsername: %s\nOrderList0:\n", user_name);
        for (int i = 0; i < num_items; i++) {
            printf("%s %d\n", items[i], quantities[i]);
        }

        for (int i = 0; i < num_stores; i++) {
            pid_t pid = fork();

            if (pid == 0) {
                char store_path[256];
                snprintf(store_path, sizeof(store_path), "%s/%s", dataset_path, stores[i]);
                handle_store(store_path, stores[i]);
                exit(0);
            } else if (pid > 0) {
                printf("PID %d create child for store %s PID:%d\n", getpid(), stores[i], pid);
            } else {
                perror("Fork failed");
                exit(1);
            }
        }

        for (int i = 0; i < num_stores; i++) {
            wait(NULL);
        }

        printf("Process for userID completed.\n");
        exit(0);
    } else if (user_pid > 0) {
        wait(NULL);
    } else {
        perror("Fork failed for userID process");
        exit(1);
    }

    printf("All processes completed successfully.\n");
    return 0;
}
