#include <xf86drm.h>
#include <gbm.h>
#include <drm/drm.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <cstdlib>

#include <sys/mman.h>
// #include <sys/stat.h>
using namespace std;



void write_ppm(const char *path, int width, int height, const uint8_t *rgb){
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    fwrite(rgb, 1, (size_t)width*height*3, f);
    fclose(f);
}


int main(int argc, char **argv){

    int width = 120, height = 120;
    uint32_t stride = 0;



    if(argc < 6){
        printf("Error,use %s <0-255>\n", argv[0]);
        printf("Error,use %s enter the type of the format\n", argv[0]);
        return 1;
    }
    uint8_t fill_value = (uint8_t)atoi(argv[1]);
    gbm_bo_format buffer_format = (gbm_bo_format)atoi(argv[2]);
    uint8_t r_val = (uint8_t)atoi(argv[3]);
    uint8_t g_val = (uint8_t)atoi(argv[4]);
    uint8_t b_val = (uint8_t)atoi(argv[5]);

    int drm_fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if(drm_fd < 0){
        printf("Openning drm fail\n");
        return 1;
    }

    gbm_device* my_gbm = gbm_create_device(drm_fd);
    gbm_bo *my_gbm_bo = gbm_bo_create(my_gbm, width, height, buffer_format, GBM_BO_USE_LINEAR);

    void* gbm_bo_map_data = nullptr;
    void* gbm_data = gbm_bo_map(my_gbm_bo, 0, 0, width, height, GBM_BO_TRANSFER_WRITE, &stride, &gbm_bo_map_data);

    printf("here\n");
    for(int y = 0; y < height; y++){
        uint8_t* row = (uint8_t*)gbm_data + (y * stride);
        for(int x = 0; x < width; x++){
            uint8_t* px = row + x * 4;
            px[0] = b_val;
            px[1] = g_val;
            px[2] = r_val;
        }
    }
    printf("here2\n");


uint8_t* rgb_buf = new uint8_t[width * height * 3];

    for (int y = 0; y < height; y++) {
        uint8_t* row = (uint8_t*)gbm_data + y * stride;
        for (int x = 0; x < width; x++) {
            uint8_t* px = row + x * 4;          // BGRX in memory: px[0]=B, px[1]=G, px[2]=R
            uint8_t* dst = rgb_buf + (y * width + x) * 3;
            dst[0] = px[2];   // R
            dst[1] = px[1];   // G
            dst[2] = px[0];   // B
        }
    }

    write_ppm("/media/abdu/LinuxHome/Embedded_Linux/git_ignoring/gsoc/learning/13-drm/img", 120, 120, rgb_buf);
    delete[] rgb_buf;


    int plans = gbm_bo_get_plane_count(my_gbm_bo);

    for(int p = 0; p < plans; p++){
        uint32_t offset= gbm_bo_get_offset(my_gbm_bo, p);
        uint32_t plan_stride = gbm_bo_get_stride_for_plane(my_gbm_bo, p);
        int plan_fd = gbm_bo_get_fd_for_plane(my_gbm_bo, p);
        printf("plane %d: offset=%u stride=%u fd=%d\n", p, offset, plan_stride, plan_fd);
    }

    uint64_t modifier = gbm_bo_get_modifier(my_gbm_bo);
    printf("modifier: 0x%lx\n", modifier);
    
    // gbm_bo_get_modifier(bo);

    printf("I have %d plans\n", plans);


    int gbm_bo_fd = gbm_bo_get_fd(my_gbm_bo);
    void* read_data = mmap(nullptr, stride * height, PROT_READ, MAP_SHARED, gbm_bo_fd, 0);

    bool mismatch = false;
    for(int y = 0; y < height; y++){
        uint8_t* row = (uint8_t*)read_data + (y * stride);
        for(int x = 0; x < stride; x++){
            if(row[x] != fill_value){
                printf("ERROR mismatch data \n");
                mismatch = true;
                break;
            }
        }
        if(mismatch){
            break;
        }
    }

    if(mismatch){
        printf("ERROR mismatch data \n");
    }
    
    printf("DONE\n");
}