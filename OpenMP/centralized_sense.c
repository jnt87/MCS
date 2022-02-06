#include <stdbool.h>
#include <omp.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

int P;

#define DEBUG 0

void centralized_barrier(int * count, bool * sense, bool * local_sense) {
    *local_sense = !*local_sense;
    int temp_count = __sync_fetch_and_sub(count, 1);
    if(temp_count == 1) {
        *count = P;
        *sense = *local_sense;
    } else {
        while (*sense != *local_sense);
    }
    return;
}

int main(int argc, char **argv) {
    int count = 0;    
    bool sense = true; 
    P = 9; //number of threads
    double total_time = 0.0;
    if(argc > 1) {
        P = atoi(argv[1]);
    }
    int num_of_tests = 100000;
    omp_set_dynamic(0);
    omp_set_num_threads(P);
    count = P;
    printf("CSR num threads: %d\n", P);
    #pragma omp parallel shared(count, sense) firstprivate(P)
    {
        bool local_sense = true;
        int tid = omp_get_thread_num();
        int i;
        int num_exec = 0;
        struct timeval start, finish;
        double td = 0;
        double tdf = 0;
        double tds = 0;
        gettimeofday(&start, NULL);
        for(i = 0; i < num_of_tests; i++) { //run sense barrier across all threads i number of times
            centralized_barrier(&count, &sense, &local_sense);
            num_exec++;
        }
        gettimeofday(&finish, NULL);

        tds = ((start.tv_sec * 1e9) + (start.tv_usec * 1e3));
        tdf = ((finish.tv_sec * 1e9) + (finish.tv_usec * 1e3));
        //printf("num_exec for tid %d: %d\n", tid, num_exec);
        td = (tdf - tds) / (num_of_tests) ;

        printf("average time (ns): %f on thread: %d\n", td, tid);
        #pragma omp critical
        total_time += td;
    }
    printf("avg time in all threads in ms: %f\n", total_time/P);
}

