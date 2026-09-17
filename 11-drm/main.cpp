#include <fstream>
#include <stdio.h>
#include <xf86drm.h>
#include <fcntl.h>    // For open()
#include <gbm.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
using namespace std;


int main(){
    int height = 120, width = 120;
    int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if(fd == -1){
        printf("Error openning the file \n");
    }
    drmVersionPtr mine;
    mine = drmGetVersion(fd);
    printf("maj: %d, min: %d, name: %s, desc: %s\n", mine->version_major, mine->version_minor, mine->name, mine->desc);

    drmFreeVersion(mine);


    gbm_device* my_device;
    my_device = gbm_create_device(fd);

    int supported;

    supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
    printf("\nGBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING : %d\n", supported);

    supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_ARGB8888, GBM_BO_USE_RENDERING);
    printf("\nGBM_FORMAT_ARGB8888, GBM_BO_USE_RENDERING : %d\n", supported);

    supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_NV12, GBM_BO_USE_RENDERING);
    printf("\nGBM_FORMAT_NV12, GBM_BO_USE_RENDERING : %d\n", supported);

    supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_XRGB8888, GBM_BO_USE_LINEAR);
    printf("\nGBM_FORMAT_XRGB8888, GBM_BO_USE_LINEAR : %d\n", supported);

    supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_ARGB8888, GBM_BO_USE_LINEAR);
    printf("\nGBM_FORMAT_ARGB8888, GBM_BO_USE_LINEAR : %d\n", supported);

    supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_NV12, GBM_BO_USE_LINEAR);
    printf("\nGBM_FORMAT_NV12, GBM_BO_USE_LINEAR : %d\n", supported);


    gbm_bo* my_bo = gbm_bo_create(my_device, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_LINEAR);
    uint32_t my_stride=0;
    void *map_data = NULL;

    void* map_handle = gbm_bo_map(my_bo, 0, 0, width, height, GBM_BO_TRANSFER_WRITE, &my_stride, &map_data);
    
    for (int y = 0; y < 120; y++) { 
        uint8_t *row = (uint8_t*)map_handle + y * my_stride;
        row[0] = (uint8_t)y ;
    }

    gbm_bo_unmap(my_bo, map_data);


    int dma_fd = gbm_bo_get_fd(my_bo);

    size_t buf_size = (size_t)my_stride * height;
    void *check = mmap(nullptr, buf_size, PROT_READ, MAP_SHARED, dma_fd, 0);

    if (dma_fd == -1){
        printf("error fstat\n");
    }

    bool ok =true;
    uint8_t *bytes = (uint8_t *)check;

    for(int y = 0; y < height; y++){
        if(bytes[y*my_stride] != y){
            printf("THERE IS A MISMATCH !!!\n");
            ok = false;
            break;
        }
    }

    if(ok){
        printf("Everything went well !\n");
    }

    munmap(check, buf_size);
    close(dma_fd);
    gbm_bo_destroy(my_bo);
    gbm_device_destroy(my_device);
    close(fd);
    return 0;
}