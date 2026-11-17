#include "coll_comm.h"

bool InitComm(const CommConfig& config, CollComm& comm){
    if(comm.ready){
        return false;
    }

    if (config.group.empty() || config.rankSize <= 0 ||
    config.myRank < 0 || config.myRank >= config.rankSize ||
    config.queueCapacity == 0) {
    return false;
    }

    comm.config = config;
    comm.opQueue.clear();
    comm.head = 0;
    comm.expand = 0;
    comm.ready = true;
    return true;
};

bool CollComm::GetAsyncConfig(bool& enabled, std::size_t& capacity) const {
    enabled = config.asyncSubmitEnabled;
    capacity = config.queueCapacity;
    return true;
}