#include "producer.hpp"
#include <fstream>
#include <jsoncpp/json/json.h>
#include <coda/logger.h>
#include <coda/error.hpp>

namespace nnxcam {

int vprod_lockmgr_cb(void **mutex, enum AVLockOp op)
{
    if (!mutex)
    {
        return -1;
    }

    switch(op)
    {
    case AV_LOCK_CREATE:
    {
        *mutex = nullptr;
        auto m = new std::mutex();
        *mutex = static_cast<void*>(m);
        break;
    }
    case AV_LOCK_OBTAIN:
    {
        auto m = static_cast<std::mutex*>(*mutex);
        m->lock();
        break;
    }
    case AV_LOCK_RELEASE:
    {
        auto m = static_cast<std::mutex*>(*mutex);
        m->unlock();
        break;
    }
    case AV_LOCK_DESTROY:
    {
        auto m = static_cast<std::mutex*>(*mutex);
        delete m;
        break;
    }
    default:;
    }
    return 0;
}

void Producer::init()
{
    av_register_all();
    av_lockmgr_register(&vprod_lockmgr_cb);
    avformat_network_init();
}

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
    const auto shm_key = json_config["shm_key"].asString();
    uint32_t shm_queue_size = json_config["queue_size"].asUInt();
    _shm_queue.connect(shm_key.c_str(), shm_queue_size);
    auto& cameras_json = json_config["cameras"];
    for (Json::Value::ArrayIndex i = 0; i != cameras_json.size(); i++)
    {
        auto& camera_json = cameras_json[i];
        CameraConfig camera_config;
        camera_config.id = i;
        camera_config.url = camera_json["url"].asString();
        _cameras.emplace_back(camera_config);
    }
    return true;
}

void Producer::run()
{
    Reader _main_reader(_cameras[0], _queue_lock, &_shm_queue);
    if (!_main_reader.init())
    {
        throw coda_error("Can't init video reader");
    }
    for (size_t i = 1; i < _cameras.size(); i++)
    {
        auto& camera_info = _cameras[i];
        auto reader = std::make_shared<Reader>(camera_info, _queue_lock, &_shm_queue);
        if (!reader->init())
        {
            throw coda_error("Can't init video reader");
        }
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
