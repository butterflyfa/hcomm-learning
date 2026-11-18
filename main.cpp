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

ProgressResult ExecuteAllReduceSum(const AllReduceParams& rank0, const AllReduceParams& rank1, std::size_t& next0 , std::size_t next1, FaultPoint& faultPoint){
    if (rank0.count != rank1.count ||
    rank0.reduceOp != ReduceOp::SUM ||
    rank1.reduceOp != ReduceOp::SUM) {
    return ProgressResult::ERROR;
}

    if (rank0.count > 0 &&
    (!rank0.input || !rank1.input || !rank0.output || !rank1.output)) {
    return ProgressResult::ERROR;
}

    if(faultPoint == FaultPoint::BEFORE_EXECUTE){
        faultPoint = FaultPoint::NONE;
        return ProgressResult::RETRYABLE_ERROR;
    }
    // 如果说是在执行之前就报错，那就返回一个重执行的信号

    //TODO: cuda做并行运算，用gpu并行计算各元素的和
    for(std::size_t i =0; i<rank0.count;i++){
        int sum = rank0.input[i] + rank1.input[i];
        rank0.output[i] = sum;
        rank1.output[i] = sum;

        next0 = i + 1;
        next1 = i + 1;

        if(faultPoint == FaultPoint::AFTER_FIRST_ELEMENT && i == 0 && next0 < rank0.count){
            faultPoint = FaultPoint::NONE;
            return ProgressResult::RETRYABLE_ERROR;
        }
    }
    return ProgressResult::COMPLETED;
}

ProgressResult TryRunNextAllReduce(CollComm& rank0, CollComm& rank1, FaultPoint& failOnce){

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

    if(failOnce == FaultPoint::AFTER_FIRST_ELEMENT || failOnce == FaultPoint::BEFORE_EXECUTE){
        failOnce = FaultPoint::NONE; // 表示故障已触发
        return ProgressResult::RETRYABLE_ERROR;
    }

    if(!ExecuteAllReduceSum(*op0.allReduceParams, *op1.allReduceParams, failOnce)){
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