#include "producer.hpp"
#include <fstream>
#include <jsoncpp/json/json.h>

namespace aux {

bool Producer::read_config(const std::string& config_path)
{
    Json::Value json_config;
    Json::CharReaderBuilder json_builder;
    std::ifstream config_fs(config_path, std::ifstream::binary);
    std::string errs;
    try
    {
        bool ok = Json::parseFromStream(json_builder, config_fs, &json_config, &errs);
        if (!ok)
        {
            //log_error("Can't parse module json config:\n%s\n", errs.c_str());
            return false;
        }
    }
    catch(std::exception& exc)
    {
        //log_error("Exception occured when loading module:\n%s\n", exc.what());
        return false;
    }
}

}
