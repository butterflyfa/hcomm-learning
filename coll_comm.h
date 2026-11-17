#pragma once

#include <cstddef>
#include <deque>
#include <string>

// ----------------- 算子 ------------------
class OpRecord { 
public:
    OpRecord(const std::string& name) :name(name){}
    std::string name;
};

// ----------------- 通信域 ------------------
struct CommConfig{
    std::string group; // 通讯域的名称
    int myRank; // 该对象（卡）在这个通信域的组内编号
    int rankSize; 
    std::size_t queueCapacity = 0;
    bool asyncSubmitEnabled = false; // 检查该通信域是否开启异步模式
}; 

struct CollComm{ // 一个rank在本地持有的通信域对象,用于记录自己的算子队列和执行进度
    CommConfig config{};
    std::deque<OpRecord> opQueue;
    std::size_t head = 0; // 已经确定执行完成的位置
    std::size_t expand = 0; // 展开的位置
    bool ready = false; // 判断是否已经初始化

    bool GetAsyncConfig(bool& enabled, std::size_t& capacity) const;
};

bool InitComm(const CommConfig& config, CollComm& comm);