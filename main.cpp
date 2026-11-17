#include <cstddef>
#include <deque>
#include <iostream>
#include <string>
#include "coll_comm.h"

bool TryEnqueue(std::deque<OpRecord>& queue, const OpRecord& op) {
    constexpr std::size_t capacity = 2;
    if (queue.size() >= capacity) {
        return false;
    }
    queue.push_back(op);
    return true;
}

bool Submit(std::deque<OpRecord>& deviceQueue, const OpRecord& op) {
    bool accepted = TryEnqueue(deviceQueue, op);
if (accepted) {
        std::cout << op.name << " 已接收\n";
    } else {
        std::cout << op.name << " 队满\n";
    }
    return accepted;
}

bool TryUnfoldNext(const std::deque<OpRecord>& opQueue, std::size_t& expand) {
    if (expand >= opQueue.size()) {
        return false;
    }
    std::cout << "展开 " << opQueue[expand].name << '\n';
    ++expand;
    return true;
}

int main() {
    std::deque<OpRecord> opQueue;
    std::size_t expand = 0;
    Submit(opQueue, OpRecord("Op1"));
    Submit(opQueue, OpRecord("Op2"));
    Submit(opQueue, OpRecord("Op3"));


    std::cout << "expand = " << expand << '\n';
    std::cout << "队列长度 = " << opQueue.size() << '\n';
    std::cout << "队首算子 = " << opQueue.front().name << '\n';
    return 0;
}