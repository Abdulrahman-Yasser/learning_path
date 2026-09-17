#include <iostream>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>


int main()
{
    libcamera::CameraManager my_cm;
    my_cm.start();

    auto cams = my_cm.cameras();
    for(auto i = cams.begin(); i < cams.end(); i++){
        printf("%s\n", (*i)->id().c_str());
    }

    cams.at(0)->acquire();
    for(const auto &prop : cams.at(0)->properties()){
        printf("new prop: %s : %s\n",   libcamera::properties::properties.at(prop.first)->name().c_str(),
                                        prop.second.toString().c_str());
    }
    cams.at(0)->release();
    cams.clear();
    my_cm.stop();

}
