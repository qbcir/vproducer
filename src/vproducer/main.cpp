#include "producer.hpp"
#include <string>
#include <getopt.h>

int main(int argc, char **argv)
{
    int c;
    std::string config_file;
    while ((c = getopt(argc, argv, "c:")) != -1)
    {
        switch (c)
        {
            case 'c':
                config_file = optarg;
                break;
        }
    }
    nnxcam::Producer::init();
    nnxcam::Producer vprod;
    vprod.read_config(config_file);
    vprod.run();
    return 0;
}
