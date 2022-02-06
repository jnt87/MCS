#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/time.h>
#include <omp.h>
#include <mpi.h>

struct treenode {
    int have_child[4];
    int wake_child[2];
    int parent;
    int wake_parent;
    //size to cacheline
};

static struct treenode *tnode;

void mcs(int jrank) {
    int i;
    for(i = 0; i < 4; ++i) {
        if(tnode->have_child[i] != -1) {
            MPI_Recv(NULL, 0, MPI_BYTE, tnode->have_child[i], 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } 
    if(jrank != 0) {
        MPI_Send(NULL, 0, MPI_BYTE, tnode->parent, 2, MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_BYTE, tnode->wake_parent, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    for(i = 0; i < 2; ++i) {
        if(tnode->wake_child[i] != -1) {
            MPI_Send(NULL, 0, MPI_BYTE, tnode->wake_child[i], 1, MPI_COMM_WORLD);
        }
    }
}

void mcs_sense(int jrank, int num_threads, int *count, bool *sense, bool *local_sense) {
    *local_sense = !*local_sense;
    int temp_count = __sync_fetch_and_sub(count, 1);
    if(temp_count == 1) {
        *count = num_threads;
        mcs(jrank);
        *sense = *local_sense;
    } else {
        while (*sense != *local_sense);
    }
    return;
}

int main(int argc, char** argv) {
    int num_of_tests = 10000;

    int count = 0;
    bool sense = true; 
    double total_time = 0.0;
    int num_threads = 2;
    if(argc > 1) {
        num_threads = atoi(argv[1]);
    }
    omp_set_dynamic(0);
    omp_set_num_threads(num_threads);
    count = num_threads;

    int num_procs;
    int j_aka_rank;

    // enable MPI thread support
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    if (provided != MPI_THREAD_SERIALIZED) {
        printf("Required thread support wasn't provided\n");
        return -1;
    }

    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &j_aka_rank);
    printf("num procs: %d\n", num_procs);
    
    tnode = (struct treenode*) malloc(sizeof(struct treenode));
    // initialize spins 
    for(int i = 0; i < 4; ++i) {
        if(4 * j_aka_rank + i + 1 < num_procs) { // intuitive to add one to LHS
        // compilation will optimize the 4*jrank and 2*jrank
            tnode->have_child[i] = 4 * j_aka_rank + i + 1;
        } else {
            tnode->have_child[i] = -1;
        }
    }

    // initialize wakeup
    for(int i = 0; i < 2; ++i) {
        if(2 * j_aka_rank + i + 1 < num_procs) {
            tnode->wake_child[i] = 2 * j_aka_rank + i + 1;
        } else {
            tnode->wake_child[i] = -1;
        }
    }

    // initialize parents
    if (j_aka_rank != 0) {
        tnode->parent = (j_aka_rank - 1) / 4;
        tnode->wake_parent = (j_aka_rank - 1) / 2;
    } else {
        tnode->parent = -1;
        tnode->wake_parent = -1;
    }
    
    #pragma omp parallel 
    {
        bool local_sense = true;
        int tid = omp_get_thread_num();
        struct timeval start, finish;

        gettimeofday(&start, NULL);
        for(int i = 0; i < num_of_tests; i++) {
            // printf("thread %d on proc %d arrived\n", tid, j_aka_rank);
            mcs_sense(j_aka_rank, num_threads, &count, &sense, &local_sense);
            // printf("thread %d on proc %d left\n", tid, j_aka_rank);
            /*if (i % 1000 == 0) {
                printf("reached %d\n", i);
            }*/
        }
        gettimeofday(&finish, NULL);

        double tds = ((start.tv_sec * 1e9) + (start.tv_usec * 1e3));
        double tdf = ((finish.tv_sec * 1e9) + (finish.tv_usec * 1e3));
        double td = (tdf - tds) / (num_of_tests);

        printf("Average time (ns):%f on thread: %d on proc: %d\n", td, tid, j_aka_rank);
        #pragma omp critical
        total_time += td;
    }
    printf("Average time (ns): %f on proc: %d\n", total_time / num_threads, j_aka_rank);

    
    MPI_Finalize();
    free(tnode);
    return(0);
}
