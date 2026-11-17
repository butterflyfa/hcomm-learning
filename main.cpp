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
    // cuda做并行运算
    for(int i =0; i<rank0.count;i++){
        int sum = rank0.input[i] + rank1.input[i];
        rank0.output[i] = sum;
        rank1.output[i] = sum;
    }
    return true;
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