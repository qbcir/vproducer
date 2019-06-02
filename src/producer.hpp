#ifndef __NNXCAM_VPROD_PRODUCER_HPP_21897__
#define __NNXCAM_VPROD_PRODUCER_HPP_21897__

#include <vector>
#include <thread>

namespace aux {

class Producer
{
public:
    bool read_config(const std::string& config_path);
private:
};

}

#endif /* __NNXCAM_VPROD_PRODUCER_HPP_21897__ */
