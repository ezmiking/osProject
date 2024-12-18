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
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>

char items[256][256];
int quantities[256];
int num_items = 0;

pthread_mutex_t cart_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t value_cond = PTHREAD_COND_INITIALIZER;
pthread_rwlock_t file_rwlock = PTHREAD_RWLOCK_INITIALIZER;

typedef struct {
    float total_price;
    int check_in_out;
    float value;
    char name[256][256];
    float price[100];
    float score[100];
    int entity[100];
    int name_count;
} cart_shop;

int shm_id;
int product_threads_done = 0;
int total_product_threads = 0;
pthread_mutex_t thread_count_lock = PTHREAD_MUTEX_INITIALIZER;

bool value_thread_done = false;

const char *dataset_path = "./Dataset";
const char *stores[] = {"Store1", "Store2", "Store3"};
int num_stores = 3;

//neveshtan .log
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pid_t global_user_pid;
char global_user_name[256];

static void write_to_log(const char *log_file_path, const char *format, ...) {
    va_list args;
    va_start(args, format);

    pthread_mutex_lock(&log_lock);
    FILE *logf = fopen(log_file_path, "a");
    if (logf) {
        vfprintf(logf, format, args);
        fclose(logf);
    }
    pthread_mutex_unlock(&log_lock);

    va_end(args);
}

int update_entity_in_file(const char *filepath, int purchased_amount);
int update_score_in_file(const char *filepath, float user_score);
bool find_and_update_product_file(const char *store_path, const char *product_name, int purchased_amount);
bool find_and_update_product_score(const char *store_path, const char *product_name, float user_score);

int update_entity_in_file(const char *filepath, int purchased_amount) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("Failed to open product file to update");
        return -1;
    }

    char *lines[1024];
    int line_count = 0;
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), f)) {
        lines[line_count] = strdup(buffer);
        line_count++;
    }
    fclose(f);

    int entity_line_index = -1;
    int current_entity = -1;
    for (int i = 0; i < line_count; i++) {
        int temp_ent;
        if (sscanf(lines[i], " Entity: %d", &temp_ent) == 1) {
            entity_line_index = i;
            current_entity = temp_ent;
            break;
        }
    }

    if (entity_line_index == -1) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        return -1;
    }

    int new_entity = current_entity - purchased_amount;
    if (new_entity < 0) new_entity = 0;

    char new_entity_line[256];
    snprintf(new_entity_line, sizeof(new_entity_line), " Entity: %d\n", new_entity);
    free(lines[entity_line_index]);
    lines[entity_line_index] = strdup(new_entity_line);

    f = fopen(filepath, "w");
    if (!f) {
        perror("Failed to open product file to write updated data");
        for (int i = 0; i < line_count; i++) free(lines[i]);
        return -1;
    }

    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], f);
        free(lines[i]);
    }
    fclose(f);

    return 0;
}

int update_score_in_file(const char *filepath, float user_score) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("Failed to open product file to update score");
        return -1;
    }

    char *lines[1024];
    int line_count = 0;
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), f)) {
        lines[line_count] = strdup(buffer);
        line_count++;
    }
    fclose(f);

    int score_line_index = -1;
    float current_score = -1.0f;
    for (int i = 0; i < line_count; i++) {
        float temp_score;
        if (sscanf(lines[i], " Score: %f", &temp_score) == 1) {
            score_line_index = i;
            current_score = temp_score;
            break;
        }
    }

    if (score_line_index == -1) {
        current_score = 0.0f;
        score_line_index = line_count;
        line_count++;
        lines[score_line_index] = strdup("");
    }

    float new_score = (current_score + user_score) / 2.0f;

    char new_score_line[256];
    snprintf(new_score_line, sizeof(new_score_line), " Score: %.2f\n", new_score);
    free(lines[score_line_index]);
    lines[score_line_index] = strdup(new_score_line);

    f = fopen(filepath, "w");
    if (!f) {
        perror("Failed to open product file to write updated score");
        for (int i = 0; i < line_count; i++) free(lines[i]);
        return -1;
    }

    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], f);
        free(lines[i]);
    }
    fclose(f);

    return 0;
}

bool find_and_update_product_file(const char *store_path, const char *product_name, int purchased_amount) {
    DIR *dir = opendir(store_path);
    if (!dir) return false;

    struct dirent *entry;
    bool updated = false;
    struct stat st;

    while ((entry = readdir(dir)) != NULL && !updated) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", store_path, entry->d_name);

        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            DIR *cat_dir = opendir(path);
            if (!cat_dir) continue;
            struct dirent *file_entry;
            while ((file_entry = readdir(cat_dir)) != NULL && !updated) {
                if (strcmp(file_entry->d_name, ".") == 0 || strcmp(file_entry->d_name, "..") == 0)
                    continue;
                char file_path[512];
                snprintf(file_path, sizeof(file_path), "%s/%s", path, file_entry->d_name);

                FILE *temp_f = fopen(file_path, "r");
                if (!temp_f) continue;

                char line[256];
                bool found_name = false;
                while (fgets(line, sizeof(line), temp_f) != NULL) {
                    char temp_name[256];
                    if (sscanf(line, " Name: %255[^\n]", temp_name) == 1) {
                        if (strcmp(temp_name, product_name) == 0) {
                            found_name = true;
                            break;
                        }
                    }
                }
                fclose(temp_f);

                if (found_name) {
                    pthread_rwlock_wrlock(&file_rwlock);
                    int res = update_entity_in_file(file_path, purchased_amount);
                    pthread_rwlock_unlock(&file_rwlock);

                    if (res == 0) {
                        updated = true;
                    }
                }
            }
            closedir(cat_dir);
        }
    }

    closedir(dir);
    return updated;
}

bool find_and_update_product_score(const char *store_path, const char *product_name, float user_score) {
    DIR *dir = opendir(store_path);
    if (!dir) return false;

    struct dirent *entry;
    bool updated = false;
    struct stat st;
    while ((entry = readdir(dir)) != NULL && !updated) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", store_path, entry->d_name);

        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            DIR *cat_dir = opendir(path);
            if (!cat_dir) continue;
            struct dirent *file_entry;
            while ((file_entry = readdir(cat_dir)) != NULL && !updated) {
                if (strcmp(file_entry->d_name, ".") == 0 || strcmp(file_entry->d_name, "..") == 0)
                    continue;
                char file_path[512];
                snprintf(file_path, sizeof(file_path), "%s/%s", path, file_entry->d_name);

                FILE *temp_f = fopen(file_path, "r");
                if (!temp_f) continue;

                char line[256];
                bool found_name = false;
                while (fgets(line, sizeof(line), temp_f) != NULL) {
                    char temp_name[256];
                    if (sscanf(line, " Name: %255[^\n]", temp_name) == 1) {
                        if (strcmp(temp_name, product_name) == 0) {
                            found_name = true;
                            break;
                        }
                    }
                }
                fclose(temp_f);

                if (found_name) {
                    pthread_rwlock_wrlock(&file_rwlock);
                    int res = update_score_in_file(file_path, user_score);
                    pthread_rwlock_unlock(&file_rwlock);

                    if (res == 0) {
                        updated = true;
                    }
                }
            }
            closedir(cat_dir);
        }
    }

    closedir(dir);
    return updated;
}

void *select_best_cart(void *args) {
    cart_shop *store_carts = (cart_shop *)args;
    float max_value = -1;
    int best_cart_index = -1;

    pthread_mutex_lock(&cart_lock);
    for (int i = 0; i < num_stores; i++) {
        if (store_carts[i].check_in_out == 1 && store_carts[i].value > max_value) {
            max_value = store_carts[i].value;
            best_cart_index = i;
        }
    }

    for (int i = 0; i < num_stores; i++) {
        if (i != best_cart_index && store_carts[i].check_in_out == 1) {
            store_carts[i].check_in_out = 0;
        }
    }

    pthread_mutex_unlock(&cart_lock);

    return (void*)(long)best_cart_index;
}

void *final_thread_func(void *args) {
    struct {
        cart_shop *store_carts;
        int best_cart_index;
        char budget_input[256];
    } *final_args = args;

    cart_shop *store_carts = final_args->store_carts;
    int best_cart_index = final_args->best_cart_index;

    if (best_cart_index < 0) {
        printf("No best cart to finalize.\n");
        free(final_args);
        pthread_exit(NULL);
    }

    cart_shop *best_cart = &store_carts[best_cart_index];
    const char *best_store_name = stores[best_cart_index];

    char store_path[256];
    snprintf(store_path, sizeof(store_path), "%s/%s", dataset_path, best_store_name);

    for (int i = 0; i < best_cart->name_count; i++) {
        char *product_name = best_cart->name[i];
        int purchased_amount = 0;
        for (int j = 0; j < num_items; j++) {
            if (strcmp(items[j], product_name) == 0) {
                purchased_amount = quantities[j];
                break;
            }
        }

        if (purchased_amount > 0) {
            printf("Final thread: Updating entity for product %s by %d\n", product_name, purchased_amount);
            bool res = find_and_update_product_file(store_path, product_name, purchased_amount);
            if (!res) {
                printf("Warning: Could not update entity for product %s\n", product_name);
            } else {
                printf("Entity for product %s updated successfully.\n", product_name);
            }
        }
    }

    free(final_args);
    pthread_exit(NULL);
}

void *score_thread_func(void *args) {
    struct {
        cart_shop *store_carts;
        int best_cart_index;
    } *score_args = args;

    cart_shop *store_carts = score_args->store_carts;
    int best_cart_index = score_args->best_cart_index;

    if (best_cart_index < 0) {
        printf("No best cart to score.\n");
        free(score_args);
        pthread_exit(NULL);
    }

    cart_shop *best_cart = &store_carts[best_cart_index];
    const char *best_store_name = stores[best_cart_index];

    char store_path[256];
    snprintf(store_path, sizeof(store_path), "%s/%s", dataset_path, best_store_name);

    for (int i = 0; i < best_cart->name_count; i++) {
        char *product_name = best_cart->name[i];
        float user_score;
        printf("Please enter your score for product %s: ", product_name);
        scanf("%f", &user_score);

        printf("Score thread: Updating score for product %s by averaging with current score.\n", product_name);
        bool res = find_and_update_product_score(store_path, product_name, user_score);
        if (!res) {
            printf("Warning: Could not update score for product %s\n", product_name);
        } else {
            printf("Score for product %s updated successfully.\n", product_name);
        }
    }

    free(score_args);
    pthread_exit(NULL);
}

struct product_thread_args {
    char *file_path;
    cart_shop *store_cart;
    char log_file_path[512];
};

void *handle_product(void *args) {
    struct product_thread_args *thread_args = args;

    char *path = thread_args->file_path;
    cart_shop *store_cart = thread_args->store_cart;
    char log_file_path[512];
    strcpy(log_file_path, thread_args->log_file_path);

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("PID %d TID:%ld - File %s not found\n", getpid(), syscall(SYS_gettid), path);
        write_to_log(log_file_path, "PID %d TID:%ld - File %s not found\n", getpid(), (long)syscall(SYS_gettid), path);
        free(thread_args->file_path);
        free(thread_args);
        pthread_exit(NULL);
    }

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
                        store_cart->value += temp_price * temp_score;

                        for (int j = 0; j < num_items; j++) {
                            if (strcmp(items[j], temp_name) == 0) {
                                if (temp_entity >= quantities[j]) {
                                    store_cart->total_price += quantities[j] * temp_price;
                                } else {
                                    store_cart->total_price += temp_entity * temp_price;
                                    store_cart->check_in_out = 0;
                                    printf("Store PID %d doesn't have enough quantity for %s. Requested: %d, Available: %d\n",
                                           getpid(), temp_name, quantities[j], temp_entity);
                                    write_to_log(log_file_path,
                                                 "Store PID %d doesn't have enough quantity for %s. Requested: %d, Available: %d\n",
                                                 getpid(), temp_name, quantities[j], temp_entity);
                                }
                                break;
                            }
                        }

                        printf("PID %d TID:%ld found product: %s, Price: %.2f, Score: %.2f, Entity: %d\n",
                               getpid(), syscall(SYS_gettid), temp_name, temp_price, temp_score, temp_entity);
                        write_to_log(log_file_path,
                                     "PID %d TID:%ld found product: %s, Price: %.2f, Score: %.2f, Entity: %d\n",
                                     getpid(), (long)syscall(SYS_gettid), temp_name, temp_price, temp_score, temp_entity);

                        store_cart->name_count++;
                        pthread_mutex_unlock(&cart_lock);
                    }
                    break;
                }
            }
        }
    }

    fclose(file);
    free(thread_args->file_path);
    free(thread_args);

    pthread_mutex_lock(&thread_count_lock);
    product_threads_done++;
    pthread_mutex_unlock(&thread_count_lock);

    pthread_exit(NULL);
}

void handle_category(const char *category_path, cart_shop *store_cart) {
    char cat_log_path[512];
    snprintf(cat_log_path, sizeof(cat_log_path), "%s/%d_%s.log", category_path, global_user_pid, global_user_name);
    FILE *cat_logf = fopen(cat_log_path, "w");
    if (cat_logf) fclose(cat_logf);

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

        struct product_thread_args *thread_args = malloc(sizeof(*thread_args));
        thread_args->file_path = file_path;
        thread_args->store_cart = store_cart;
        strcpy(thread_args->log_file_path, cat_log_path);

        pthread_mutex_lock(&thread_count_lock);
        total_product_threads++;
        pthread_mutex_unlock(&thread_count_lock);

        if (pthread_create(&threads[thread_count], NULL, handle_product, thread_args) != 0) {
            perror("Failed to create thread");
            free(thread_args->file_path);
            free(thread_args);
            continue;
        }

        pid_t tid = syscall(SYS_gettid);
        write_to_log(cat_log_path, "PID %d TID:%d created for product file %s\n", getpid(), (int)tid, file_path);

        thread_count++;
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    closedir(dir);
}

void handle_store(const char *store_path, const char *store_name, const char *budget_input, cart_shop *store_cart) {
    // create .log for each store
    char store_log_path[512];
    snprintf(store_log_path, sizeof(store_log_path), "%s/%d_%s.log", store_path, global_user_pid, global_user_name);
    FILE *store_logf = fopen(store_log_path, "w");
    if (store_logf) fclose(store_logf);

    write_to_log(store_log_path, "PID %d create child for store %s\n", getpid(), store_name);

    memset(store_cart, 0, sizeof(cart_shop));
    store_cart->check_in_out = 1;

    DIR *dir = opendir(store_path);
    if (dir == NULL) {
        perror("Failed to open store directory");
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
            handle_category(category_path, store_cart);
            exit(0);
        } else if (pid > 0) {
            printf("PID %d create child for category %s PID:%d\n", getpid(), entry->d_name, pid);
            write_to_log(store_log_path, "PID %d create child for category %s PID:%d\n", getpid(), entry->d_name, pid);
        } else {
            perror("Fork failed");
            exit(1);
        }
    }

    while (wait(NULL) > 0);

    closedir(dir);

    printf("\nSummary of cart for store %s:\n", store_name);
    for (int i = 0; i < store_cart->name_count; i++) {
        printf("Product: %s, Price: %.2f, Score: %.2f, Entity: %d\n",
               store_cart->name[i], store_cart->price[i], store_cart->score[i], store_cart->entity[i]);
    }
    printf("Total Value: %.2f\n", store_cart->value);
    printf("Total Price for requested items: %.2f\n", store_cart->total_price);

    if (store_cart->total_price > atof(budget_input)) {
        store_cart->check_in_out = 0;
        printf("Store %s exceeds the price threshold. Setting check_in_out to 0.\n", store_name);
    }

    printf("Check-in-out status: %d\n", store_cart->check_in_out);
}

int main() {
    printf("Username: ");
    scanf("%s", global_user_name);

    global_user_pid = getpid();

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
    budget_input[strcspn(budget_input, "\n")] = '\0';

    printf("\nUsername: %s\nOrderList0:\n", global_user_name);
    for (int i = 0; i < num_items; i++) {
        printf("%s %d\n", items[i], quantities[i]);
    }

    shm_id = shmget(IPC_PRIVATE, num_stores * sizeof(cart_shop), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Failed to create shared memory");
        exit(1);
    }

    cart_shop *shared_store_carts = (cart_shop *)shmat(shm_id, NULL, 0);
    if (shared_store_carts == (void *)-1) {
        perror("Failed to attach shared memory in main");
        exit(1);
    }

    pid_t user_pid = getpid();
    for (int i = 0; i < num_stores; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            cart_shop *store_carts = (cart_shop *)shmat(shm_id, NULL, 0);
            if (store_carts == (void *)-1) {
                perror("Failed to attach shared memory in store process");
                exit(1);
            }

            char store_path[256];
            snprintf(store_path, sizeof(store_path), "%s/%s", dataset_path, stores[i]);
            handle_store(store_path, stores[i], budget_input, &store_carts[i]);

            shmdt(store_carts);
            exit(0);
        } else if (pid > 0) {
            printf("PID %d create child for store %s PID:%d\n", user_pid, stores[i], pid);
            // creat .log
        } else {
            perror("Fork failed");
            exit(1);
        }
    }

    while (wait(NULL) > 0);

    pthread_t val_thread;
    void *best_cart_index_ptr;
    pthread_create(&val_thread, NULL, select_best_cart, shared_store_carts);
    pthread_join(val_thread, &best_cart_index_ptr);

    int best_cart_index = (int)(long)best_cart_index_ptr;

    struct {
        cart_shop *store_carts;
        int best_cart_index;
        char budget_input[256];
    } *final_args = malloc(sizeof(*final_args));
    final_args->store_carts = shared_store_carts;
    final_args->best_cart_index = best_cart_index;
    strcpy(final_args->budget_input, budget_input);

    pthread_t final_thread;
    pthread_create(&final_thread, NULL, final_thread_func, final_args);
    pthread_join(final_thread, NULL);

    struct {
        cart_shop *store_carts;
        int best_cart_index;
    } *score_args = malloc(sizeof(*score_args));
    score_args->store_carts = shared_store_carts;
    score_args->best_cart_index = best_cart_index;

    pthread_t score_thread;
    pthread_create(&score_thread, NULL, score_thread_func, score_args);
    pthread_join(score_thread, NULL);

    for (int i = 0; i < num_stores; i++) {
        printf("Final status of store %s: check_in_out = %d, value = %.2f\n",
               stores[i], shared_store_carts[i].check_in_out, shared_store_carts[i].value);
    }

    if (shmdt(shared_store_carts) == -1) {
        perror("Failed to detach shared memory in main");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Failed to remove shared memory");
    }

    printf("Process for userID completed.\n");
    printf("All operations completed successfully.\n");
    return 0;
}
