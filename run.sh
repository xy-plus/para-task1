set -e

module load mpi/mpich-3.2-x86_64
mpicc -o main.out main.c -std=c99
# mpiexec -n 2 ./main.out
# mpiexec -n 10 ./main.out bcast 10000000
mpiexec -n 10 ./main.out gather 10000000

# export PATH=~/mpich/v4.0a2/bin:$PATH
# mpicc -o main.out main.c
# mpiexec -n 2 ./main.out

# ~/mpich/v4.0a2/bin/mpicc -o d.out deadlock.c
# ~/mpich/v4.0a2/bin/mpiexec ./d.out
