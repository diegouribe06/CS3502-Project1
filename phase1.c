# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <unistd.h>

// Configuration - experiment with different values
# define NUM_ACCOUNTS 2
# define NUM_THREADS 4
# define TRANSACTIONS_PER_THREAD 10
# define INITIAL_BALANCE 1000.0
// Account data structure ( GIVEN )
typedef struct {
    int account_id ;
    double balance ;
    int transaction_count ;
} Account;

// Global shared array - THIS CAUSES RACE CONDITIONS!
Account accounts[NUM_ACCOUNTS];
int adjustment = 0; //also decided to use another global varianble to keep track of depostis and withdrawals
//it causes more race conditions, but since that's the point of this phase i'm assuming it's fine

//Both functions below were modified to keep track of deposits and withdrawals to make the final count work
// GIVEN: Example deposit function WITH race condition
double deposit_unsafe(int account_id, double amount) {
    // READ
    double current_balance = accounts[account_id].balance;

    // MODIFY (simulate processing time)
    usleep(1);
    double new_balance = current_balance + amount;

    //WRITE (another thread might have changed balance between READ and WRITE!)
    accounts[account_id].balance = new_balance;
    accounts[account_id].transaction_count++;
    adjustment += amount; // Track total deposits for this account

    return amount;
}

//TODO 1: Implement withdrawal_unsage() following the same pattern
//Reference: Copy the structure of deposit_unsafe() above
//Question: What's different between deposit and withdrawal?
double withdrawal_unsafe(int account_id, double amount) {
    double current_balance = accounts[account_id].balance;

    usleep(1);
    double new_balance = current_balance - amount;

    accounts[account_id].balance = new_balance;
    accounts[account_id].transaction_count++;
    adjustment -= amount; // Track total withdrawals for this account

    return amount;
}

//TODO 2: Implement the thread function
//Reference: See OSTEP Ch. 27 for pthread function signature
//Reference: Appendix A.2 for void* parameter explanation
void* teller_thread(void* arg) {
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
            deposit_unsafe(account_idx, amount);
            printf("Teller %d: Deposited $%.2f to Account %d\n", teller_id, amount, account_idx);
        }
        else {
            withdrawal_unsafe(account_idx, amount);
            printf("Teller %d: Withdrew $%.2f from Account %d\n", teller_id, amount, account_idx);
        }
        
    }
    return NULL;
}

//TODO 3: Implement main function
//Reference: See pthread_create and pthread_join man pages
int main() {

    //timing stuff from phase 2
    struct timespec start, end;;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("=== Phase 1: Race Conditions Demo ===\n\n");

    //TODO 3a: Initialize all accounts
    //Hint: Loop through accounts array
    //Set: account_id = i, balance = INITIAL_BALANCE, transaction_count = 0

    //YOUR CODE HERE
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
    }

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
    int thread_ids[NUM_THREADS];;   //GIVEN: Separate array for IDs

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
        //YOUR pthradjustmentead_join CODE HERE
        pthread_join(threads[i], NULL);
    }


    //TODO 3f: Calculate and display results
    printf("\n=== Final Results ===\n");
    double actual_total = adjustment;

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf("Account %d: $%.2f (%d transactions)\n", i, accounts[i].balance, accounts[i].transaction_count);
        actual_total += accounts[i].balance;
    }

    printf("\nExpected Total: $%.2f\n", expected_total);
    printf("Actual Total:   $%.2f\n", actual_total);
    printf("Difference:     $%.2f\n", actual_total - expected_total);

    //TODO 3g: Add race condition detection message
    //IF expected != actual, print "RACE CONDITION DETECTED!"
    //Instruct user to run multiple times

    if (actual_total != expected_total) {
        printf("\nRACE CONDITION DETECTED!\n");
    }

    //more phase 2 timing stuff
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.4f seconds\n", elapsed_time);

    return 0;
}