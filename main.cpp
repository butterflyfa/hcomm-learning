#include <cstddef>
#include <deque>
#include <iostream>
#include <string>
#include "coll_comm.h"
#include "op_type.h"

bool TryEnqueue(CollComm& comm, const OpRecord& op) {
    const std::size_t capacity = comm.config.queueCapacity;
    if (comm.opQueue.size() >= capacity) {
        return false;
    }
    comm.opQueue.push_back(op);
    return true;
}

bool Submit(CollComm& comm, const OpRecord& op) {
    if(comm.ready == false)
        return false;

    return TryEnqueue(comm, op);    
}

bool SubmitAllReduce(CollComm& comm, const AllReduceParams& params){
    OpRecord op("AllReduce"); // 定义一个算子，名字叫Allreduce
    op.type = OpType::ALLREDUCE;
    op.allReduceParams = params;
    return Submit(comm, op);
}


bool TryUnfoldNext(CollComm& comm) {
    const std::size_t index = comm.expand - comm.head;
    if (index >= comm.opQueue.size()) {
        return false;
    }
    // 算子展开
    std::cout << "展开 " << comm.opQueue[index].name << '\n';
    ++comm.expand;
    return true;
}

bool TryConfirmNext(CollComm& comm){ // 收到完成通知后，回收已展开的算子记录
    if(!comm.ready || comm.head == comm.expand) 
        return false;
    else{
        comm.opQueue.pop_front();
        comm.head++;
        return true;
    }
}

bool ExecuteAllReduceSum(const AllReduceParams& rank0, const AllReduceParams& rank1){
    if (rank0.count != rank1.count ||
    rank0.reduceOp != ReduceOp::SUM ||
    rank1.reduceOp != ReduceOp::SUM) {
    return false;
}

    if (rank0.count > 0 &&
    (!rank0.input || !rank1.input || !rank0.output || !rank1.output)) {
    return false;
}
    //TODO: cuda做并行运算，用gpu并行计算各元素的和
    for(std::size_t i =0; i<rank0.count;i++){
        int sum = rank0.input[i] + rank1.input[i];
        rank0.output[i] = sum;
        rank1.output[i] = sum;
    }
    return true;
}

ProgressResult TryRunNextAllReduce(CollComm& rank0, CollComm& rank1, bool& failOnce){

    if (!rank0.ready || !rank1.ready || // 是否已经初始化
    rank0.config.group != rank1.config.group || // 是否属于同一个通信域
    rank0.config.rankSize != 2 || rank1.config.rankSize != 2 || // 是否只允许两个设备进行通信)
    rank0.config.myRank != 0 || rank1.config.myRank != 1) // 是否传的是同一张卡
        {
    return ProgressResult::ERROR;
    }

            
    if (rank0.opQueue.empty() || rank1.opQueue.empty()){
        return ProgressResult::WAITING; // 是空的就等一下
    }
    
    // 两边都提交了记录，并且当前处理的是同一轮
    if(rank0.head != rank1.head) {
        return ProgressResult::ERROR;
    }

    const auto& op0 = rank0.opQueue.front();
    const auto& op1 = rank1.opQueue.front();

    if (!op0.allReduceParams || !op1.allReduceParams ||
    op0.type != OpType::ALLREDUCE ||
    op1.type != OpType::ALLREDUCE) {
        return ProgressResult::ERROR;
    }

    if (rank0.expand == rank0.head && !TryUnfoldNext(rank0)) { // 判断已展开的会不会和已完成的同一处，意思就是判断是不是都执行完了
        return ProgressResult::ERROR;                                           //并且就直接在判断中执行展开函数
    }
    if (rank1.head == rank1.expand && !TryUnfoldNext(rank1)) {
        return ProgressResult::ERROR;
    }

    if(failOnce == true){
        failOnce = false; // 表示故障已触发
        return ProgressResult::RETRYABLE_ERROR;
    }
    // 在执行算子之前，如果出现故障了，那就先将标志位置为false，等待下一轮的执行

    if(!ExecuteAllReduceSum(*op0.allReduceParams, *op1.allReduceParams)){
        return ProgressResult::ERROR;
    }

        // 求和成功后，回收两条记录
    const bool confirmed0 = TryConfirmNext(rank0);
    const bool confirmed1 = TryConfirmNext(rank1);

    if(confirmed0 && confirmed1){
        return ProgressResult::COMPLETED;
    }

    return ProgressResult::ERROR;
    
}


int main() {
    CollComm rank0{};

    if (!InitComm({"groupA", 0, 2, 2}, rank0)) {
        std::cerr << "初始化失败\n";
        return 1;
    }

    std::cout << rank0.config.group << " rank=" << rank0.config.myRank
            << " ready=" << std::boolalpha << rank0.ready << '\n';

    return 0;
}