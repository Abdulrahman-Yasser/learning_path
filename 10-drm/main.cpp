#include <fstream>
#include <stdio.h>
#include <xf86drm.h>
#include <fcntl.h>    // For open()
#include <gbm.h>
#include <unistd.h>
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

    gbm_device_destroy(my_device);

    close(fd);
}