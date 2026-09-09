#include <fstream>
#include <stdio.h>
#include <xf86drm.h>
#include <fcntl.h>    // For open()
#include <gbm.h>
#include <unistd.h>
#include <sys/mman.h>
using namespace std;


int main(){
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


    gbm_bo* my_bo = gbm_bo_create(my_device, 120, 120, GBM_FORMAT_XRGB8888, GBM_BO_USE_LINEAR);
    uint32_t my_stride=120 * 4;
    uint32_t *map_data ;
    gbm_bo_map(my_bo, 0, 0, 120, 120, GBM_BO_USE_LINEAR, &my_stride, (void**)&map_data);
    printf("here\n");
    gbm_bo_unmap(my_bo, (void*)map_data);

    int new_fd = gbm_bo_get_fd(my_bo);

    mmap(new_fd, );

    gbm_device_destroy(my_device);

    close(fd);
}