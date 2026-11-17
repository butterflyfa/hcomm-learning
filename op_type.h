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

struct AllReduceParams {
    const int* input;
    int* output;
    std::size_t count; // 表示参与AllReduce的元素个数
    ReduceOp reduceOp;
};