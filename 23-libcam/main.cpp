#include <iostream>
#include <unistd.h>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

#include <mutex>
#include <condition_variable>
#include <queue>

#include <iostream>
#include <cstdio>
#include <fstream>
#include <sys/mman.h>

class WarmupLatch
{
private:
    std::mutex my_mutex_;
    std::condition_variable cv_;
    bool done_ = false;

public:
    void signal()
    {
        {
            std::lock_guard<std::mutex> lock(my_mutex_);
            done_ = true;
        }
        cv_.notify_all();
    }
    void wait()
    {
        std::unique_lock<std::mutex> lock(my_mutex_);
        cv_.wait(lock, [this]
                 { return done_; });
        done_ = false;
    }
};

libcamera::CameraManager my_cm;
std::shared_ptr<libcamera::Camera> g_my_camera;
WarmupLatch my_latch;

// #include <gbm.h>
// <libcamera::Request *>
unsigned int g_width, g_height, g_stride;

static uint8_t clamp(float v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)(v + 0.5f));
}

void save_img(const uint8_t *data, int width, int height, int stride)
{
    FILE *f = fopen("img_right.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", width, height);

    for (int y = 0; y < height; y++)
    {
        const uint8_t *row = data + (size_t)y * stride;
        for (int x = 0; x < width; x += 2)
        {
            const uint8_t *g = row + x * 2;
            float y0 = g[0], u = g[1] - 128.0f, y1 = g[2], v = g[3] - 128.0f;

            uint8_t out[6] = {
                clamp(y0 + 1.402 * v), clamp(y0 - 0.344 * u - 0.714 * v), clamp(y0 + 1.772 * u),
                clamp(y1 + 1.402 * v), clamp(y1 - 0.344 * u - 0.714 * v), clamp(y1 + 1.772 * u)};
            fwrite(out, 1, 6, f);
        }
    }
}

static void request_is_complete(libcamera::Request *m)
{
    printf("request_is_complete called !!!!\n");
    if (m->status() == libcamera::Request::RequestCancelled)
    {
        printf("request canceled\n");
        return;
    }
    else if (m->status() == libcamera::Request::RequestPending)
    {
        printf("request_is_pending\n");
        return;
    }

    const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> buffers = m->buffers();
    printf("request is : %s\n", m->toString().c_str());

    for (auto x : buffers)
    {
        libcamera::FrameBuffer *b = x.second;
        const libcamera::FrameMetadata &metadata = b->metadata();

        unsigned int nplanes = 0;
        std::cout << " seq: " << metadata.sequence << " - byte used: ";
        for (const libcamera::FrameMetadata::Plane &p : metadata.planes())
        {
            std::cout << p.bytesused;
            if (++nplanes < metadata.planes().size())
                std::cout << "/";
        }
        std::cout << std::endl;
        if (metadata.sequence == 12)
        {
            for (auto p : b->planes())
            {
                void *data = mmap(nullptr, p.length, PROT_READ, MAP_SHARED, p.fd.get(), p.offset);
                if(data != MAP_FAILED){
                    save_img((const uint8_t*)data, g_width, g_height, g_stride);
                    munmap(data, p.length);
                }
            }
        }
    }

    m->reuse(libcamera::Request::ReuseBuffers);
    g_my_camera->queueRequest(m);
    printf("signaling\n");
    my_latch.signal();
}

int main()
{
    my_cm.start();

    auto cameras = my_cm.cameras();

    if (cameras.size() == 0)
    {
        printf("NO CAMERA EXIST\n");
    }
    auto my_camera = cameras.at(0);
    g_my_camera = my_camera;
    auto config = my_camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
    if (config->validate() != 0)
    {
        printf("configuration error\n");
    }

    g_width = config->at(0).size.width;
    g_height = config->at(0).size.height;
    g_stride = config->at(0).stride;
    my_camera->acquire();
    my_camera->configure(config.get());

    auto my_stream = config->at(0).stream();
    auto alloc = libcamera::FrameBufferAllocator(my_camera);

    alloc.allocate(my_stream);

    const auto &frame_buf = alloc.buffers(my_stream);

    std::vector<std::unique_ptr<libcamera::Request>> requests;
    for (const auto &x : frame_buf)
    {
        printf("frame status %d\n", x->metadata().status);
        printf("frame timestamp %ld\n", x->metadata().timestamp);
        printf("frame cookie %ld\n", x->cookie());
        for (auto i : x->planes())
        {
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
    for (auto &r : requests)
    {
        my_camera->queueRequest(r.get());
    }
    while (1)
    {
        printf("waiting\n");
        my_latch.wait();
    }
    my_camera->stop();
}
