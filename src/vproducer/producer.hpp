#ifndef __NNXCAM_VPROD_PRODUCER_HPP_21897__
#define __NNXCAM_VPROD_PRODUCER_HPP_21897__

#include <mutex>
#include <vector>
#include <thread>

#include "reader.hpp"
#include "common/shm_queue.hpp"

namespace nnxcam {

class Producer
{
public:
    static void init();
    bool read_config(const std::string& config_path);
    void run();
private:
    std::mutex _queue_lock;
    std::vector<CameraConfig> _cameras;
    std::vector<std::shared_ptr<Reader>> _readers;
    std::vector<std::thread> _threads;
    ShmQueue _shm_queue;
};

}

#endif /* __NNXCAM_VPROD_PRODUCER_HPP_21897__ */
