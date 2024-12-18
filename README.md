# README

## Introduction
This project simulates a multi-process and multi-threaded shopping environment. A "User" process creates orders and receives input from the user, while multiple "Store" processes, and their "Category" subprocesses, process product data. For products, threads are used to handle the details. Additionally, several threads are created after all processing to determine the best cart, update inventory, and finally update product scores.

## Architecture and Flow
1. **User Process (Main Process)**:  
   - Prompts the user for their username, a list of desired items and quantities, and a price threshold.
   - Spawns multiple **Store Processes**, each handling one store directory.
   - Waits for all store processes to complete.
   - Creates a `value_thread` to select the best cart among all stores.
   - Once the best cart is determined, creates a `final_thread` to update the product entities (inventory) in the corresponding files.
   - After updating inventory, creates a `score_thread` to prompt the user for a score for each purchased product and updates the product files with the new average score.

2. **Store Processes**:  
   - Each store process is created by the user process.
   - For each category within the store directory, a **Category subprocess** is created.
   - Categories do not run in separate threads but as separate child processes.

3. **Category Processes**:  
   - Each category process scans its directory for product files.
   - For each product file, **Product Threads** are created to process product data (Name, Price, Score, and Entity).
   - These threads populate the store's cart with product information and calculate total price and value. They also adjust the `check_in_out` flag if inventory is insufficient.

4. **Shared Memory**:  
   - Shared memory is used to store an array of `cart_shop` structures, one for each store.
   - Each store process attaches to this shared memory segment and updates its corresponding cart.
   - After all stores have finished processing, the user process uses the collected data to determine the best cart.

5. **Final Steps (Threads Created by User Process)**:
   - **value_thread**: Chooses the best cart (highest value) among those with `check_in_out == 1`, then sets `check_in_out` of the others to `0`.
   - **final_thread**: Updates the inventory (`Entity`) of each purchased product by editing the corresponding product files.
   - **score_thread**: After updating inventory, prompts the user for a score for each purchased product, averages it with the existing score in the product file, and updates the file.

## Logging
- In each directory (store and category), a log file named `UserID_UserName.log` is created.
- `UserID` corresponds to the `PID` of the user process.
- `UserName` is the username provided by the user.
- All process and thread creation events, as well as product findings, are logged into the corresponding directory's log file.  
- Logging is protected by a mutex (`log_lock`) to ensure thread-safe writes.

## Synchronization and Concurrency
- **Processes**: Created with `fork()`.
- **Threads**: Created with `pthread_create()`.
- **Shared Memory**: Created with `shmget()` and accessed via `shmat()`.
- **Mutexes and Condition Variables**:  
  - `pthread_mutex_t` to protect critical sections when modifying shared data structures (e.g., `cart_lock`).  
  - `pthread_cond_t` for signaling when tasks complete if needed.  
  - `pthread_rwlock_t` used to ensure that only one writer thread at a time updates product files while allowing multiple readers if needed.
- **Resource Cleanup**:  
  - All threads are joined with `pthread_join()`.
  - All processes wait for their children with `wait()`.
  - Shared memory is detached and removed after use.

## How to Run
1. Ensure the `Dataset` directory structure is set up as follows:
