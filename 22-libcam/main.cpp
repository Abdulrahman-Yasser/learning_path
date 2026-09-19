#include <iostream>
#include <unistd.h>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

libcamera::CameraManager my_cm;
std::shared_ptr<libcamera::Camera> g_my_camera ;

// #include <gbm.h>
// <libcamera::Request *>
static void request_is_complete(libcamera::Request *m){
    printf("request_is_complete called !!!!\n");
    if(m->status() == libcamera::Request::RequestCancelled){
        printf("request canceled\n");
        return;
    }else if(m->status() == libcamera::Request::RequestPending){
        printf("request_is_pending\n");
        return;
    }

    const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> buffers = m->buffers();
    printf("request is : %s\n", m->toString().c_str());

    for(auto x : buffers){
        libcamera::FrameBuffer *b = x.second;
        const libcamera::FrameMetadata &metadata = b->metadata();

        unsigned int nplanes = 0;
        std::cout << " seq: " << metadata.sequence << " - byte used: ";
        for (const libcamera::FrameMetadata::Plane &p : metadata.planes()){
            std::cout << p.bytesused;
            if(++nplanes < metadata.planes().size()) std::cout << "/";
        }
        std::cout << std::endl;
    }

    m->reuse(libcamera::Request::ReuseBuffers);
    g_my_camera->queueRequest(m);
}


int main(){
    my_cm.start();

    auto cameras = my_cm.cameras();

    if(cameras.size() == 0){
        printf("NO CAMERA EXIST\n");
    }
    auto my_camera = cameras.at(0);
    g_my_camera = my_camera;
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

    std::vector<std::unique_ptr<libcamera::Request>> requests;
    for(const auto &x : frame_buf){
        printf("frame status %d\n", x->metadata().status);
        printf("frame timestamp %ld\n", x->metadata().timestamp);
        printf("frame cookie %ld\n", x->cookie());
        for(auto i : x->planes()){
            printf("fd num %d\n", i.fd.get());
            printf("length num %d\n", i.length);
            printf("offset num %d\n", i.offset);
        }
        // printf("frame num %s\n", x->request()->toString().c_str());


        auto request = my_camera->createRequest();
        request->addBuffer(my_stream, x.get());
        requests.push_back(std::move(request));
    }

    

    auto &buffer0 = frame_buf[0];

    // my_camera->requestCompleted.connect();
    my_camera->requestCompleted.connect(request_is_complete);

    printf("%ld\n", frame_buf.size());


    my_camera->start();
    for(auto &r : requests){
        my_camera->queueRequest(r.get());
    }
    sleep(6);
    my_camera->stop();
}
