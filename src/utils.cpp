#include "utils.hpp"
#include <chrono>

namespace aux {

int64_t clock_get_milliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

}
