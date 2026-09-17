#include <iostream>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

// #include <gbm.h>


int main(){
    libcamera::CameraManager my_cm;
    my_cm.start();

    auto cameras = my_cm.cameras();

    if(cameras.size() == 0){
        printf("NO CAMERA EXIST\n");
    }
    auto my_camera = cameras.at(0);
    auto config = my_camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
    if(config->validate() != 0){
        printf("configuration error\n");
    }

    my_camera->acquire();
    my_camera->configure(config.get());


    auto my_stream = config->at(0).stream();
    auto alloc = libcamera::FrameBufferAllocator(my_camera);

    alloc.allocate(my_stream);

    const auto &frame_buf = alloc.buffers(my_stream);

    for(const auto &x : frame_buf){
        printf("frame num %d\n", x->metadata().status);
        printf("frame num %ld\n", x->metadata().timestamp);
        printf("frame num %ld\n", x->cookie());
        for(auto i : x->planes()){
            printf("fd num %d\n", i.fd.get());
            printf("length num %d\n", i.length);
            printf("offset num %d\n", i.offset);
        }
        // printf("frame num %s\n", x->request()->toString().c_str());
    }

    

    auto &buffer0 = frame_buf[0];
    auto request = my_camera->createRequest();
    request->addBuffer(my_stream, buffer0.get());

    my_camera->requestCompleted.connect();

    printf("%ld\n", frame_buf.size());
}
