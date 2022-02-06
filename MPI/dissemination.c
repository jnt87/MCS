#include <stdbool.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct dissem_proc {
    bool *private_flags[2]; // other procs will send messages to flip these
    int *partner_ids; // this proc will send messages to others
    bool sense; // flip sense after every two barriers
    int stage; // count every two barriers
} dissem_proc_t;

int get_rounds(int num_procs) {
    return ceil(log2((double) num_procs));
}

void dissem_barrier(int pid, dissem_proc_t *proc, int num_procs) {
    bool *private_flags = proc->private_flags[proc->stage];
    int *partner_ids = proc->partner_ids;

    for (int i = 0; i < get_rounds(num_procs); i++) {
        MPI_Send(&(proc->sense), 1, MPI_INT, partner_ids[i], i, MPI_COMM_WORLD);
        while (private_flags[i] != proc->sense) {
            MPI_Recv(&(private_flags[i]), 1, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    proc->stage = (proc->stage + 1) % 2;
    if (!proc->stage) {
        proc->sense = !proc->sense;
    }
}

dissem_proc_t init_dissem_barrier_proc(int pid, int num_procs) {
    dissem_proc_t proc;
    proc.sense = true;
    proc.stage = 0;

    int rounds = get_rounds(num_procs);
    
    proc.partner_ids = calloc(rounds, sizeof(int));
    proc.private_flags[0] = calloc(rounds, sizeof(bool));
    proc.private_flags[1] = calloc(rounds, sizeof(bool));
    if (!proc.partner_ids || !proc.private_flags[0] || !proc.private_flags[1]) {
        printf("Can't allocate more memory.\n");
        exit(1);
    }

    for (int k = 0; k < rounds; k++) {
        proc.private_flags[0][k] = !proc.sense;
        proc.private_flags[1][k] = !proc.sense;

        int partner_id = (pid + (1 << k)) % num_procs;
        proc.partner_ids[k] = partner_id;
    }

    return proc;
}

void clean_dissem_barrier_proc(dissem_proc_t *proc) {
    free(proc->partner_ids);
    free(proc->private_flags[0]);
    free(proc->private_flags[1]);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int pid, num_procs;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    
    printf("dessimination barrier rank: %d\n", pid);
    int num_iter = 100000;

    // Maintain state for each proc in the barrier
    dissem_proc_t dissem_proc = init_dissem_barrier_proc(pid, num_procs);

    double start = MPI_Wtime();
    for (int i = 0; i < num_iter; i++) {
        dissem_barrier(pid, &dissem_proc, num_procs);
    }
    double finish = MPI_Wtime();

    double td = ((finish - start) * 1e6) / (num_iter);
    printf("Average time us: %f on proc: %d\n", td, pid);

    clean_dissem_barrier_proc(&dissem_proc);
    MPI_Finalize();
}

