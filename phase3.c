#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
//for error codes
#include <errno.h>

// Configuration - experiment with different values
#define NUM_ACCOUNTS 2
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Updated Account structure with mutex (GIVEN)
typedef struct {
    int account_id;
    double balance;
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

        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

int transfer(double amount, int from, int to) {
    //timer stuff for deadlock detection
    struct timespec deadlockTimer;
    clock_gettime(CLOCK_REALTIME, &deadlockTimer);
    deadlockTimer.tv_sec += 5; // Set timeout for 5 seconds
    int rc;

    //get the lock for the from acct
    pthread_mutex_lock(&accounts[from].lock);
    //make sure the acct has sufficient funds
    if (accounts[from].balance < amount) {
        pthread_mutex_unlock(&accounts[from].lock);
        return -1; // Insufficient funds
    }
    //simulating higher processing time
    //i think it is necessary to make sure the deadlock is more likely to occur
    usleep(100);

    //get the lock for the to acct
    rc = pthread_mutex_timedlock(&accounts[to].lock, &deadlockTimer);
    if (rc == ETIMEDOUT) {
        // Deadlock detected, release the first lock and return error
        pthread_mutex_unlock(&accounts[from].lock);
        return -2; // error code for deadlock
    }
    else if (rc != 0) {
        // Some other error occurred
        pthread_mutex_unlock(&accounts[from].lock);
        return -3; // error code for other mutex error
    }

    accounts[from].balance -= amount;
    accounts[from].transaction_count++;
    accounts[to].balance += amount;
    accounts[to].transaction_count++;

    //unlock both accounts
    pthread_mutex_unlock(&accounts[from].lock);
    pthread_mutex_unlock(&accounts[to].lock);
    return 0;
}


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
        int account_idx, account_idy;
        do {
            account_idx = rand_r(&seed) % NUM_ACCOUNTS;
            account_idy = rand_r(&seed) % NUM_ACCOUNTS;
        } while (account_idx == account_idy); // Ensure different accounts for transfer


        //TODO 2c: Generate random amount (1-100)
        double amount = 1.0 + (rand_r(&seed) % 100);

        int transfer_result = transfer(amount, account_idx, account_idy);
        if (transfer_result == -1) {
            printf("\nAccount %d has insufficient funds for transfer of $%.2f to Account %d\n", account_idx, amount, account_idy);
        }
        else if (transfer_result == -2) {
            printf("\nTeller %d: Deadlock detected during transfer of $%.2f from Account %d to Account %d. Transaction aborted.\n", teller_id, amount, account_idx, account_idy);
        }
        else if (transfer_result == -3) {
            printf("\nTeller %d: Mutex error during transfer of $%.2f from Account %d to Account %d. Transaction aborted.\n", teller_id, amount, account_idx, account_idy);
        }
        else {
            printf("Teller %d: Transferred $%.2f from Account %d to Account %d\n", teller_id, amount, account_idx, account_idy);
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
    printf("=== Phase 3: Deadlock Demo ===\n\n");

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
        pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);

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
    }

    printf("\nExpected Total: $%.2f\n", expected_total); // Adjust expected total based on global adjustment
    printf("Actual Total:   $%.2f\n", actual_total);
    printf("Difference:     $%.2f\n", actual_total - expected_total);

    //TODO 3g: Add race condition detection message
    //IF expected != actual, print "RACE CONDITION DETECTED!"
    //Instruct user to run multiple times

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.4f seconds\n", elapsed_time);

    return 0;
}


