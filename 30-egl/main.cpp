#include <stdio.h>
#include <gbm.h>
#include <drm/drm.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

using namespace std;

int main(){
    int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    gbm_device* my_gbm_dev = gbm_create_device(fd);
    
    PFNEGLGETPLATFORMDISPLAYEXTPROC my_egl_get_display_ext = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    EGLDisplay my_egl_display = my_egl_get_display_ext(EGL_PLATFORM_GBM_MESA, my_gbm_dev, nullptr);

    // Error checking
    if(my_egl_display == EGL_NO_DISPLAY){
        printf("my_egl_get_display_ext bug\n");
    }

    if(eglInitialize(my_egl_display, nullptr, nullptr) == EGL_FALSE){
        printf("eglInitialize bug\n");
    }
    
    if( eglBindAPI(EGL_OPENGL_API) == EGL_FALSE){
        printf("eglInitialize bug\n");
    }

    const EGLint frameBffer_attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, 
        EGL_BLUE_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_NONE
    };
    EGLConfig frameBffer_configs;
    EGLint num_of_configs;
    if( eglChooseConfig(my_egl_display, frameBffer_attributes, &frameBffer_configs, 1, &num_of_configs) == EGL_FALSE){
        printf("eglChooseConfig bug\n");
    }

    const EGLint surface_attributes[] = {
        EGL_WIDTH, 480,
        EGL_HEIGHT, 480,
        EGL_NONE
    };
    EGLSurface my_egl_surface = eglCreatePbufferSurface(my_egl_display, frameBffer_configs, surface_attributes);
    if(my_egl_surface == EGL_NO_SURFACE){
        printf("eglCreatePbufferSurface bug\n");
    }

    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLContext my_egl_ctx = eglCreateContext(my_egl_display, frameBffer_configs, EGL_NO_CONTEXT, context_attributes);
    if(my_egl_ctx == EGL_NO_CONTEXT){
        printf("eglCreateContext bug\n");
    }
    
    eglMakeCurrent(my_egl_display, my_egl_surface, EGL_NO_SURFACE, my_egl_ctx);
    
}