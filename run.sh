set -e

module load mpi/mpich-3.2-x86_64
mpic++ -o main.out main.cpp -std=c++11
mpiexec -n $2 ./main.out $1

# export PATH=~/mpich/v4.0a2/bin:$PATH
# mpicc -o main.out main.c
# mpiexec -n 2 ./main.out

# ~/mpich/v4.0a2/bin/mpicc -o d.out deadlock.c
# ~/mpich/v4.0a2/bin/mpiexec ./d.out
