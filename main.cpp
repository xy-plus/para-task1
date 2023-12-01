#include "reader.h"
#include <iostream>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

// 初始化数据为 0 + plus, 1 + plus, 2 + plus, ...
void init_data(int *buf, int data_size, int plus) {
    for (int i = 0; i < data_size; i++) {
        buf[i] = i + plus;
    }
}

// 检查数据是否正确，是否为 plus, plus + mul, plus + 2 * mul, ...
int check_data(int *buf, int data_size, int plus, int mul = 1) {
    for (int i = 0; i < data_size; i++) {
        if (buf[i] != i * mul + plus) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    KeyValueParser parser(argv[1]);
    parser.parse();
    MPI_Init(&argc, &argv);

    int MYID, NPROC;
    int ROOTID = atoi(parser.data().at("root_id").c_str());
    MPI_Comm_rank(MPI_COMM_WORLD, &MYID);
    MPI_Comm_size(MPI_COMM_WORLD, &NPROC);

    // 打印 config
    if (MYID == ROOTID) {
        cout << "======== CONFIG ========" << endl;
        for (const auto &kv : parser.data()) {
            std::cout << kv.first << ": " << kv.second << std::endl;
        }
        cout << "========================" << endl;
    }
    string operation = parser.data().at("operation");
    int data_size = atoi(parser.data().at("data_size").c_str());

    // 数据准备
    int *sendbuf = nullptr;
    int *recvbuf = nullptr;

    double start_time, end_time;
    // 记录开始时间
    start_time = MPI_Wtime();

    // 选择和执行集合通信操作
    if (operation == "bcast") {
        recvbuf = (int *)malloc(data_size * sizeof(int));
        // 只有 ROOT 进程初始化数据
        if (MYID == ROOTID) {
            init_data(recvbuf, data_size, MYID);
        }
        MPI_Bcast(recvbuf, data_size, MPI_INT, ROOTID, MPI_COMM_WORLD);
    } else if (operation == "gather") {
        sendbuf = (int *)malloc(data_size * sizeof(int));
        // 只有 ROOT 进程接收数据，需要 NPROC 倍于发送数据的空间
        if (MYID == ROOTID) {
            recvbuf = (int *)malloc(NPROC * data_size * sizeof(int));
        }
        init_data(sendbuf, data_size, MYID);
        MPI_Gather(sendbuf, data_size, MPI_INT, recvbuf, data_size, MPI_INT, ROOTID, MPI_COMM_WORLD);
    } else if (operation == "reduce_sum") {
        sendbuf = (int *)malloc(data_size * sizeof(int));
        // 非根节点的 recvbuf 也需要初始化，否则会报错，尽管不会被使用
        recvbuf = (int *)malloc(data_size * sizeof(int));
        if (MYID == ROOTID) {
            // 根节点的数据 send data 为 MYID, MYID + 1, MYID + 2, ...
            init_data(sendbuf, data_size, MYID);
        } else {
            // 非根节点的数据 send data 为 MYID, MYID, MYID, ...
            for (int i = 0; i < data_size; i++) {
                sendbuf[i] = MYID;
            }
        }
        MPI_Reduce(sendbuf, recvbuf, data_size, MPI_INT, MPI_SUM, ROOTID, MPI_COMM_WORLD);
    } else if (operation == "all_reduce_sum") {
        sendbuf = (int *)malloc(data_size * sizeof(int));
        recvbuf = (int *)malloc(data_size * sizeof(int));
        if (MYID == ROOTID) {
            // 根节点的数据 send data 为 MYID, MYID + 1, MYID + 2, ...
            init_data(sendbuf, data_size, MYID);
        } else {
            // 非根节点的数据 send data 为 MYID, MYID, MYID, ...
            for (int i = 0; i < data_size; i++) {
                sendbuf[i] = MYID;
            }
        }
        MPI_Allreduce(sendbuf, recvbuf, data_size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    } else if (operation == "scan_sum") {
        sendbuf = (int *)malloc(data_size * sizeof(int));
        recvbuf = (int *)malloc(data_size * sizeof(int));
        init_data(sendbuf, data_size, MYID);
        MPI_Scan(sendbuf, recvbuf, data_size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    } else if (operation == "all_to_all") {
        sendbuf = (int *)malloc(data_size * NPROC * sizeof(int));
        recvbuf = (int *)malloc(data_size * NPROC * sizeof(int));
        // 每个进程的 sendbuf 都是 0, 1, 2, ... , data_size * NPROC - 1
        init_data(sendbuf, data_size * NPROC, 0);
        MPI_Alltoall(sendbuf, data_size, MPI_INT, recvbuf, data_size, MPI_INT, MPI_COMM_WORLD);
    } else {
        // printf("Unknown operation: %s\n", operation);
        cout << "Unknown operation: " << operation << endl;
    }
    // 记录通信结束时间
    end_time = MPI_Wtime();

    // 检查数据正确性并输出带宽
    if (operation == "bcast") {
        // 每个进程单独检查接受到的数据是否正确
        if (check_data(recvbuf, data_size, ROOTID) == 1) {
            cout << "Process " << MYID << ": data check passed." << endl;
        } else {
            cout << "Process " << MYID << ": data check failed." << endl;
        }
        if (MYID == ROOTID) {
            double bandwidth = (double)data_size * NPROC * sizeof(int) / (end_time - start_time) / 1e6;
            cout << "Operation: " << operation << ", Data Size: " << data_size << ", Bandwidth: " << bandwidth << " MB/s" << endl;
        }
    } else if (operation == "gather") {
        // 根节点检查接受到的数据是否正确
        if (MYID == ROOTID) {
            // 数据根据进程 ID 排列
            for (int i = 0; i < NPROC; i++) {
                if (check_data(recvbuf + i * data_size, data_size, i) == 0) {
                    printf("Process %d: data check failed.\n", MYID);
                    exit(0);
                }
            }
            cout << "Process " << MYID << ": data check passed." << endl;
            double bandwidth = (double)data_size * NPROC * sizeof(int) / (end_time - start_time) / 1e6;
            cout << "Operation: " << operation << ", Data Size: " << data_size << ", Bandwidth: " << bandwidth << " MB/s" << endl;
        }
    } else if (operation == "reduce_sum") {
        // 根节点检查接受到的数据是否正确
        if (MYID == ROOTID) {
            int sum = 0;
            for (int i = 0; i < NPROC; i++) {
                sum += i;
            }
            if (check_data(recvbuf, data_size, sum) == 1) {
                cout << "Process " << MYID << ": data check passed." << endl;
            } else {
                cout << "Process " << MYID << ": data check failed." << endl;
            }
            double bandwidth = (double)data_size * NPROC * sizeof(int) / (end_time - start_time) / 1e6;
            cout << "Operation: " << operation << ", Data Size: " << data_size << ", Bandwidth: " << bandwidth << " MB/s" << endl;
        }
    } else if (operation == "all_reduce_sum") {
        // 所有节点都检查接受到的数据是否正确
        int sum = 0;
        for (int i = 0; i < NPROC; i++) {
            sum += i;
        }
        if (check_data(recvbuf, data_size, sum) == 1) {
            cout << "Process " << MYID << ": data check passed." << endl;
        } else {
            cout << "Process " << MYID << ": data check failed." << endl;
        }
        if (MYID == ROOTID) {
            double bandwidth = (double)data_size * NPROC * 2 * sizeof(int) / (end_time - start_time) / 1e6;
            cout << "Operation: " << operation << ", Data Size: " << data_size << ", Bandwidth: " << bandwidth << " MB/s" << endl;
        }
    } else if (operation == "scan_sum") {
        // 所有节点都检查接受到的数据是否正确
        int sum = 0;
        for (int i = 0; i <= MYID; i++) {
            sum += i;
        }
        if (check_data(recvbuf, data_size, sum, MYID + 1) == 1) {
            cout << "Process " << MYID << ": data check passed." << endl;
        } else {
            cout << "Process " << MYID << ": data check failed." << endl;
        }
        if (MYID == ROOTID) {
            double bandwidth = (double)data_size * NPROC * sizeof(int) / (end_time - start_time) / 1e6;
            cout << "Operation: " << operation << ", Data Size: " << data_size << ", Bandwidth: " << bandwidth << " MB/s" << endl;
        }
    } else if (operation == "all_to_all") {
        // 所有节点都检查接受到的数据是否正确
        for (int i = 0; i < NPROC; i++) {
            if (check_data(recvbuf + i * data_size, data_size, MYID * data_size) == 0) {
                printf("Process %d: data check failed.\n", MYID);
                exit(0);
            }
        }
        cout << "Process " << MYID << ": data check passed." << endl;
        if (MYID == ROOTID) {
            double bandwidth = (double)data_size * NPROC * NPROC * sizeof(int) / (end_time - start_time) / 1e6;
            cout << "Operation: " << operation << ", Data Size: " << data_size << ", Bandwidth: " << bandwidth << " MB/s" << endl;
        }
    } else {
        cout << "Unknown operation: " << operation << endl;
    }

    // 清理资源
    if (sendbuf != nullptr) {
        free(sendbuf);
    }
    if (recvbuf != nullptr) {
        free(recvbuf);
    }

    MPI_Finalize();
    return 0;
}
