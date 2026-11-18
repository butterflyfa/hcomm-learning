#pragma once
#include <cstddef>

enum class OpType {
    BROADCAST,
    ALLGATHER,
    ALLREDUCE
};

enum class ReduceOp {
    SUM,
    MAX
};

enum class ProgressResult {
    WAITING,
    COMPLETED,
    ERROR,
    RETRYABLE_ERROR,  // 本次执行遇到临时故障，可以再次尝试
};


struct AllReduceParams {
    const int* input;
    int* output;
    std::size_t count; // 表示参与AllReduce的元素个数
    ReduceOp reduceOp;
};