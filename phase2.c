#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Configuration - experiment with different values
#define NUM_ACCOUNTS 2
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Updated Account structure with mutex (GIVEN)
typedef struct {
    int account_id;
    double balance;
    //added to keep track of deposits and withdrawals
    double adjustment;
    int transaction_count;
    pthread_mutex_t lock; // NEW: Mutex for this account
} Account;

// Global shared array
Account accounts[NUM_ACCOUNTS];

// GIVEN: Example of mutex initialization
void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        accounts[i].adjustment = 0.0;

        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

// GIVEN: Example deposit function WITH proper protection
void deposit_safe(int account_id, double amount) {
    // Acquire lock BEFORE accessing shared data
    pthread_mutex_lock(&accounts[account_id].lock);

    // ===== CRITICAL SECTION =====
    // Only ONE thread can execute this at a time for this account
    accounts[account_id].balance += amount;
    accounts[account_id].transaction_count++;
    accounts[account_id].adjustment += amount; // Track total deposits for this account
    // ============================

    // Release lock AFTER modifying shared data
    pthread_mutex_unlock(&accounts[account_id].lock);
}

// TODO 1: Implement withdrawal_safe() with mutex protection
// Reference: Follow the pattern of deposit_safe() above
// Remember: lock BEFORE accessing data, unlock AFTER
void withdrawal_safe(int account_id, double amount) {
    // YOUR CODE HERE
    // Hint: pthread_mutex_lock
    // Hint: Modify balance
    // Hint: pthread_mutex_unlock
    pthread_mutex_lock(&accounts[account_id].lock);

    //critical section
    accounts[account_id].balance -= amount;
    accounts[account_id].transaction_count++;
    accounts[account_id].adjustment -= amount; // Track total withdrawals for this account

    //release the lock
    pthread_mutex_unlock(&accounts[account_id].lock);

}

// TODO 2: Update teller_thread to use safe functions
// Change: deposit_unsafe -> deposit_safe
// Change: withdrawal_unsafe -> withdrawal_safe
void* teller_thread(void* arg) {
    // YOUR CODE HERE - Copy from Phase 1 and modify to use safe functions
    int teller_id = *(int*)arg; //GIVEN: Extract thread ID

    //TODO 2a: Initialize thread-safe random seed
    //Reference: Section 7.2 "Random Numbers per Thread"
    //Hint: unsigned int seed = time(NULL) ^ pthread_self();
    unsigned int seed = time(NULL) ^ pthread_self();

    for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
        //TODO 2b: Randomly select an account (0 to NUM_ACCOUNTS-1)
        //Hint: User rand_r(&seed) % NUM_ACCOUNTS
        int account_idx = rand_r(&seed) % NUM_ACCOUNTS;

        //TODO 2c: Generate random amount (1-100)
        double amount = 1.0 + (rand_r(&seed) % 100);

        //TODO 2d: Randomly choos deposit (1) or withdrawal (0)
        //Hint: rand_r(&seed) % 2
        int operation = rand_r(&seed) % 2;

        //TODO 2e: Call the appropriate function
        if (operation == 1) {
            deposit_safe(account_idx, amount);
            printf("Teller %d: Deposited $%.2f to Account %d\n", teller_id, amount, account_idx);
        }
        else {
            withdrawal_safe(account_idx, amount);
            printf("Teller %d: Withdrew $%.2f from Account %d\n", teller_id, amount, account_idx);
        }

    }
    return NULL;
}

// TODO 3: Add performance timing
// Reference: Section 7.2 "Performance Measurement"
// Hint: Use clock_gettime(CLOCK_MONOTONIC, &start);

// TODO 4: Add mutex cleanup in main()
// Reference: man pthread_mutex_destroy
// Important: Destroy mutexes AFTER all threads complete!
void cleanup_mutexes() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
}

// TODO 5: Compare Phase 1 vs Phase 2 performance
// Measure execution time for both versions
// Document the overhead of synchronization

int main() {
    // YOUR CODE HERE - Copy from Phase 1 and modify:
    // 1. Call initialize_accounts() instead of manual initialization
    // 2. Add performance timing (TODO 3)
    // 3. Call cleanup_mutexes() before returning (TODO 4)
    struct timespec start, end;;
    clock_gettime(CLOCK_MONOTONIC, &start);
    printf("=== Phase 2: Mutex Demo ===\n\n");

    //TODO 3a: Initialize all accounts
    initialize_accounts();

    //Display initial state (GIVEN)
    printf("Initial State:\n");
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf("    Account %d: $%.2f\n", i, accounts[i].balance);
    }

    //TODO 3b: Calculate expected final balance
    //Question: With random deposits/withdrawals, what should total be?
    //Hint: Total money in system should remain constant!
    double expected_total = INITIAL_BALANCE * NUM_ACCOUNTS;

    printf("\nExpected total (without adjustment): $%.2f\n\n", expected_total);

    //TODO 3c: Create thread and thread ID arrays
    //Reference man pthread_create for pthread_t type
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];;   //GIVEN: Seperate array for IDs

    //TODO 3d: Create all threads
    //Reference: man pthread_create
    //Caution: See appendix A.2 warning about passing &i in loop!
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        //YOUR pthread_create CODE HERE
        //FORMAT: pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
        if (pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    //TODO 3e: Wait for all threads to complete
    //Reference: man pthread_join
    //Question: What happens if you skip this step?
    for (int i = 0; i < NUM_THREADS; i++) {
        //YOUR pthread_join CODE HERE
        pthread_join(threads[i], NULL);
    }

    cleanup_mutexes();

    //TODO 3f: Calculate and display results
    printf("\n=== Final Results ===\n");
    double actual_total = 0.0;

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf("Account %d: $%.2f (%d transactions)\n", i, accounts[i].balance, accounts[i].transaction_count);
        actual_total += accounts[i].balance;
        expected_total += accounts[i].adjustment; // Adjust expected total based on deposits/withdrawals
    }

    printf("\nExpected Total: $%.2f\n", expected_total); // Adjust expected total based on global adjustment
    printf("Actual Total:   $%.2f\n", actual_total);
    printf("Difference:     $%.2f\n", actual_total - expected_total);

    //TODO 3g: Add race condition detection message
    //IF expected != actual, print "RACE CONDITION DETECTED!"
    //Instruct user to run multiple times

    if (actual_total != expected_total) {
        printf("\nRACE CONDITION DETECTED!\n");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.4f seconds\n", elapsed_time);

    cleanup_mutexes();

    return 0;
}


