#include "producer.hpp"
#include <fstream>
#include <jsoncpp/json/json.h>

namespace nnxcam {

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

    auto& cameras_json = json_config["cameras"];
    for (Json::Value::ArrayIndex i = 0; i != cameras_json.size(); i++)
    {
        auto& camera_json = cameras_json[i];
        CameraConfig camera_config;
        camera_config.url = camera_json["url"].asString();
        _cameras.emplace_back(camera_config);
    }
    return true;
}

void Producer::run()
{
    Reader _main_reader(_cameras[0].url, _queue_lock, _shm_queue);

    for (size_t i = 1; i < _cameras.size(); i++)
    {
        auto& camera_info = _cameras[i];
        auto reader = std::make_shared<Reader>(camera_info.url, _queue_lock, _shm_queue);
        _readers.emplace_back(reader);
        _threads.emplace_back(std::thread(&Reader::run, reader.get()));
    }

    _main_reader.run();

    for (auto& th : _threads)
    {
        th.join();
    }
}

}
