#include <mpi.h>
#include <stdio.h>

// #define buf_size  16348
// #define buf_size  16349

// #define buf_size 655
#define buf_size 65533

int main(int argc, char **argv) {
    int myid, numprocs, other;

    int sb[buf_size];
    int rb[buf_size];
    int i;
    MPI_Status status;
    MPI_Request r;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    for (i = 0; i < buf_size; i++) {
        sb[i] = myid + i;
    }

    if (myid == 0)
        other = 1;
    if (myid == 1)
        other = 0;

    // all processor send messages to its peer
    //
    fprintf(stderr, "process :%d of %d trying sending...\n", myid, numprocs);
    fflush(stderr);
    MPI_Isend(sb, buf_size, MPI_INT, other, 1, MPI_COMM_WORLD, &r);
    // MPI_Ssend(sb, buf_size, MPI_INT, other, 1, MPI_COMM_WORLD);
    // MPI_Send(sb, buf_size, MPI_INT, other, 1, MPI_COMM_WORLD);

    // all processor receive messages from its peer
    //
    fprintf(stderr, "process :%d of %d trying receiving...\n", myid, numprocs);
    fflush(stderr);
    MPI_Recv(rb, buf_size, MPI_INT, other, 1, MPI_COMM_WORLD, &status);

    fprintf(stderr, "Hello World! Process %d of %d \n", myid, numprocs);
    fflush(stdout);
    MPI_Finalize();

    return 0;
}
