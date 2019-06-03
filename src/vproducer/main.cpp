#include "producer.hpp"
#include <string>
#include <getopt.h>

int main(int argc, char **argv)
{
    int c;
    char config_file_buf [256];
    while ((c = getopt(argc, argv, "c:")) != -1)
    {
        switch (c)
        {
            case 'c':
                snprintf(config_file_buf, sizeof(config_file_buf)-1, "%s", optarg);
                break;
        }
    }
    std::string config_file(config_file_buf);
    nnxcam::Producer::init();
    nnxcam::Producer vprod;
    vprod.read_config(config_file);
    vprod.run();
    return 0;
}
