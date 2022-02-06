#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

struct treenode {
    int have_child[4];
    int wake_child[2];
    int parent;
    int wake_parent;
    //size to cacheline
};

void mcs(MPI_Comm comm, int jrank, int P, struct treenode* tnode) {
    int i, child;
    for(i = 0; i < 4; ++i) {
        child = tnode->have_child[i];
        if(child != -1) {
            MPI_Recv(NULL, 0, MPI_INT, child, 1, comm, NULL);
        }
    } 
    if(jrank != 0) {
        MPI_Send(NULL, 0, MPI_INT, tnode->parent, 1, comm);
        MPI_Recv(NULL, 0, MPI_INT, tnode->wake_parent, 2, comm, NULL);
    }
    for(i = 0; i < 2; ++i) {
        child = tnode->wake_child[i];
        if(child != -1) {
            MPI_Send(NULL, 0, MPI_INT, child, 2, comm);
        }
    }
}

int main(int argc, char** argv) {
    struct treenode *tnode;
    int P = 4;
    int i;
    int j_aka_rank;
    int num_tests = 100000;
    double t1, t2, td;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &P);
    MPI_Comm_rank(MPI_COMM_WORLD, &j_aka_rank);
    printf("mcs num threads: %d\n", P);
    tnode = (struct treenode*) malloc(sizeof(struct treenode));
    printf("reached after malloc");
    //initialization spins 
    for(i = 0; i < 4; ++i) {
        if(4 * j_aka_rank + i + 1 < P) { // intuitive to add one to LHS
        // compilation will optimize the 4*jrank and 2*jrank
            tnode->have_child[i] = 4 * j_aka_rank + i + 1;
        } else {
            tnode->have_child[i] = -1;
        }
    }

    //initialize wakeup
    for(i = 0; i < 2; ++i) {
        if(2 * j_aka_rank + i + 1 < P) {
            tnode->wake_child[i] = 2 * j_aka_rank + i + 1;
        } else {
            tnode->wake_child[i] = -1;
        }
    }

    if(j_aka_rank != 0) {
        tnode->parent = floor((j_aka_rank - 1) / 4);
        tnode->wake_parent = floor((j_aka_rank - 1) / 2);
    } else {
        tnode->parent = -1;
        tnode->wake_parent = -1;
    }
    printf("reached clock\n");
    t1 = MPI_Wtime(); 
    for(i = 0; i < 10000; ++i) {
        mcs(MPI_COMM_WORLD, j_aka_rank, P, tnode);
    }
    t2 = MPI_Wtime();
    td = ((t2 - t1) * 1e6) / num_tests;
    printf("avg time on %d used in us: %f\n", j_aka_rank, td);

    
    MPI_Finalize();
    if(tnode) {
	free(tnode);
    }
    return(0);
}
