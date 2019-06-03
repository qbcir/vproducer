#ifndef __NNXCAM_VPROD_PRODUCER_HPP_21897__
#define __NNXCAM_VPROD_PRODUCER_HPP_21897__

#include <mutex>
#include <vector>
#include <thread>

#include "reader.hpp"
#include "common/shm_queue.hpp"

namespace nnxcam {

struct CameraConfig
{
    std::string url;
};

class Producer
{
public:
    bool read_config(const std::string& config_path);
    void run();
private:
    std::mutex _queue_lock;
    std::vector<CameraConfig> _cameras;
    std::vector<std::shared_ptr<Reader>> _readers;
    std::vector<std::thread> _threads;
    std::shared_ptr<ShmQueue> _shm_queue = nullptr;
};

}

#endif /* __NNXCAM_VPROD_PRODUCER_HPP_21897__ */
