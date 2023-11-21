#include <mpi.h>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 初始化数据为 0, 1, 2, ...
void init_data(int *buf, int data_size, int seed) {
    for (int i = 0; i < data_size; i++) {
        buf[i] = i + seed;
    }
}

// 检查数据是否正确，是否为 0, 1, 2, ...
int check_data(int *buf, int data_size, int seed) {
    for (int i = 0; i < data_size; i++) {
        if (buf[i] != i + seed) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int MYID, NPROC, ROOTID;
    MPI_Comm_rank(MPI_COMM_WORLD, &MYID);
    MPI_Comm_size(MPI_COMM_WORLD, &NPROC);
    // 暂时 ROOT ID 写死为 2
    ROOTID = 2;

    // 检查命令行参数
    if (argc < 3) {
        if (MYID == 0) {
            fprintf(stderr, "Usage: %s <operation> <data_size>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    char *operation = argv[1];
    int data_size = atoi(argv[2]);

    // 数据准备
    int *sendbuf = NULL;
    int *recvbuf = NULL;

    double start_time, end_time;
    // 记录开始时间
    start_time = MPI_Wtime();

    // 选择和执行集合通信操作
    if (strcmp(operation, "bcast") == 0) {
        recvbuf = (int *)malloc(data_size * sizeof(int));
        // 只有 ROOT 进程初始化数据
        if (MYID == ROOTID) {
            init_data(recvbuf, data_size, MYID);
        }
        MPI_Bcast(recvbuf, data_size, MPI_INT, ROOTID, MPI_COMM_WORLD);
    } else if (strcmp(operation, "gather") == 0) {
        sendbuf = (int *)malloc(data_size * sizeof(int));
        // 只有 ROOT 进程接收数据，需要 NPROC 倍于发送数据的空间
        if (MYID == ROOTID) {
            recvbuf = (int *)malloc(NPROC * data_size * sizeof(int));
        }
        init_data(sendbuf, data_size, MYID);
        MPI_Gather(sendbuf, data_size, MPI_INT, recvbuf, data_size, MPI_INT,
                   ROOTID, MPI_COMM_WORLD);
    } else {
        printf("Unknown operation: %s\n", operation);
    }
    // 记录通信结束时间
    end_time = MPI_Wtime();

    // 检查数据正确性并输出带宽
    if (strcmp(operation, "bcast") == 0) {
        // 每个进程单独检查接受到的数据是否正确
        if (check_data(recvbuf, data_size, MYID) == 1) {
            printf("Process %d: data check passed.\n", MYID);
        } else {
            printf("Process %d: data check failed.\n", MYID);
        }
        if (MYID == ROOTID) {
            double bandwidth = (double)data_size * NPROC * sizeof(int) /
                               (end_time - start_time) / 1e6;
            printf("Operation: %s, Data Size: %d, Bandwidth: %f MB/s\n",
                   operation, data_size, bandwidth);
        }
    } else if (strcmp(operation, "gather") == 0) {
        // 根节点检查接受到的数据是否正确
        if (MYID == ROOTID) {
            // 数据根据进程 ID 排列
            for (int i = 0; i < NPROC; i++) {
                if (check_data(recvbuf + i * data_size, data_size, i) == 0) {
                    printf("Process %d: data check failed.\n", MYID);
                    exit(0);
                }
            }
            printf("Process %d: data check passed.\n", MYID);
            double bandwidth = (double)data_size * NPROC * sizeof(int) /
                               (end_time - start_time) / 1e6;
            printf("Operation: %s, Data Size: %d, Bandwidth: %f MB/s\n",
                   operation, data_size, bandwidth);
        }
    } else {
        printf("Unknown operation: %s\n", operation);
    }

    // 清理资源
    if (sendbuf != NULL) {
        free(sendbuf);
    }
    if (recvbuf != NULL) {
        free(recvbuf);
    }

    MPI_Finalize();
    return 0;
}
