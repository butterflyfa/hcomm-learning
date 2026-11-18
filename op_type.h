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

enum class FaultPoint { // 查看故障所在的位置
    NONE,                 // 正常执行
    BEFORE_EXECUTE,       // 开始求和之前失败
    AFTER_FIRST_ELEMENT   // 第一个元素的和写入两边后失败
};

struct AllReduceParams {
    const int* input;
    int* output;
    std::size_t count; // 表示参与AllReduce的元素个数
    ReduceOp reduceOp;
};