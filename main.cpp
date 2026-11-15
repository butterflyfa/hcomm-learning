#include <iostream>
#include <queue>
#include <string>


class OpRecord{
public:
    OpRecord(std::string name){
        this->name = name;
    }
    std::string name;
};

    bool TryEnqueue(std::queue<OpRecord>& queue, const OpRecord& op){
        constexpr std::size_t capacity = 2;
    if(queue.size() >= capacity)
        return false;
    else
        {
            queue.push(op);
            return true;
        }
    }

int main(){
    std::queue<OpRecord> opQueue;

    std::cout << "Op1: " << TryEnqueue(opQueue, OpRecord("Op1")) << '\n';
    std::cout << "Op2: " << TryEnqueue(opQueue, OpRecord("Op2")) << '\n';
    std::cout << "Op3: " << TryEnqueue(opQueue, OpRecord("Op3")) << '\n';



    std::cout << "队列长度 = " << opQueue.size() << '\n' <<std::endl;
    std::cout << "队首算子 = " << opQueue.front().name << '\n' <<std::endl;

    return 0;
}