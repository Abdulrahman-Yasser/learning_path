#include <iostream>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>


int main()
{
    libcamera::CameraManager my_cm;
    my_cm.start();
    auto cam = my_cm.cameras().at(0);
    auto role = libcamera::StreamRole::Viewfinder;
    auto config = cam->generateConfiguration({role});



    if(config == nullptr){
        printf("config is null\n");
    }else{
        printf("config : %s\n", config->at(0).toString().c_str());
    }

    printf("config size : %s\n", config->at(0).size.toString().c_str());
    printf("config colorSpace : %s\n", config->at(0).colorSpace->toString().c_str());
    printf("config bufferCount : %d\n", config->at(0).bufferCount);
    printf("config pixelFormat : %s\n", config->at(0).pixelFormat.toString().c_str());
    printf("config frameSize : %d\n", config->at(0).frameSize);
    // printf("config : %s\n", config->at(0).toString().c_str());


    cam->acquire();
    printf("validate : %d\n",config->validate());
    cam->configure(config.get());

    cam->release();
    cam.reset();
    config.reset();
    my_cm.stop();

}
