#include <stdbool.h>
#include <omp.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct dissem_thread {
    bool *private_flags[2]; // other threads will flip these
    bool **partner_flags[2]; // this thread will flip these for others
    bool sense; // flip sense after every two barriers
    int stage; // count every two barriers
} dissem_thread_t;

int get_rounds(int num_threads) {
    return ceil(log2((double) num_threads));
}

void dissem_barrier(int tid, dissem_thread_t *threads, int num_threads) {
    dissem_thread_t *thread = &(threads[tid]);
    bool *private_flags = thread->private_flags[thread->stage];
    bool **partner_flags = thread->partner_flags[thread->stage];

    for (int i = 0; i < get_rounds(num_threads); i++) {
        *(partner_flags[i]) = thread->sense;
        while (private_flags[i] != thread->sense);
    }

    thread->stage = (thread->stage + 1) % 2;
    if (!thread->stage) {
        thread->sense = !thread->sense;
    }
}

dissem_thread_t *init_dissem_barrier_threads(int num_threads) {
    dissem_thread_t *threads = calloc(num_threads, sizeof(dissem_thread_t));
    int rounds = get_rounds(num_threads);
    // Initialize thread structures
    for (int i = 0; i < num_threads; i++) {
        dissem_thread_t *thread = &(threads[i]);
        thread->sense = true;
        thread->stage = 0;
        for (int j = 0; j < 2; j++) {
            thread->private_flags[j] = calloc(rounds, sizeof(bool));
            thread->partner_flags[j] = calloc(rounds, sizeof(bool *));
            if (!thread->private_flags[j] || !thread->partner_flags[j]) {
                printf("Can't allocate more memory.\n");
                exit(1);
            }
        }
    }
    // Now that all thread structures are initialized, point partner flags to
    // correct threads
    for (int i = 0; i < num_threads; i++) {
        dissem_thread_t *thread = &(threads[i]);
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < rounds; k++) {
                thread->private_flags[j][k] = !thread->sense;

                int partner_ind = (i + (1 << k)) % num_threads;            
                dissem_thread_t *partner = &(threads[partner_ind]);
                bool *partner_flag = &(partner->private_flags[j][k]);
                thread->partner_flags[j][k] = partner_flag;
            }
        }
    }

    return threads;
}

// void *clean_dissem_barrier_threads(dissem_thread_t *threads, int num_threads) {
//     int rounds = get_rounds(num_threads);
//     dissem_thread_t dissem_threads[num_threads] = {{0}};
//     for (int i = 0; i < desired_num_threads; i++) {

//         dissem_threads[i].private_flags = 
//     }

// }

int main(int argc, char **argv) {
    int num_threads = 8;
    if (argc > 1) {
        num_threads = atoi(argv[1]);
    } 
    printf("dissemination num threads: %d\n", num_threads);
    int num_iter = 100000;
    double total_time = 0.0;

    omp_set_dynamic(0);
    omp_set_num_threads(num_threads);

    // Maintain state for each thread in the barrier
    dissem_thread_t *dissem_threads = init_dissem_barrier_threads(num_threads);

    #pragma omp parallel 
    {
        int tid = omp_get_thread_num();
        struct timeval start, finish;

        gettimeofday(&start, NULL);
        for (int i = 0; i < num_iter; i++) {
            dissem_barrier(tid, dissem_threads, num_threads);
            //if (i % 1000 == 0) {
            //    printf("thread %d reached iteration %d\n", tid, i);
            //}
        }
        gettimeofday(&finish, NULL);

        double tds = (start.tv_sec * 1e6) + start.tv_usec;
        double tdf = (finish.tv_sec * 1e6) + finish.tv_usec;
        double td = (tdf - tds) / (num_iter);
        printf("Average time (nano s): %f on thread: %d\n", td * 1e3, tid);

        #pragma omp critical
        total_time += td;
    }

    // clean_dissem_barrier_threads(dissem_threads, num_threads);
    printf("Average time (nano s): %f\n", (total_time/num_threads) * 1e3);
}

