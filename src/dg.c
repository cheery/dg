#include <DG/dg.h>

#include "bcm_host.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "EGL/eglext_brcm.h"

#include <pthread.h>

#define SHARED_SURFACE 1
#define NATIVE_SURFACE 2
struct _dg_surface {
    union {
        char id[DG_SURFACE_ID_LENGTH];
        int global_image[5];
        EGL_DISPMANX_WINDOW_T window;
    } data;
    int type;
    EGLSurface eglsurface;
    EGLConfig eglconfig;
};

struct _dg_context {
    int api;
    EGLContext eglcontext;
    EGLConfig eglconfig;
};

static void __attribute__ ((constructor)) dgInit(void);
static void __attribute__ ((destructor))  dgScrap(void);

static EGLDisplay _egldisplay = 0;

void dgInit() {
    bcm_host_init();
    _egldisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(_egldisplay, NULL, NULL);
}

void dgScrap() {
    eglTerminate(_egldisplay);
    bcm_host_deinit();
}

typedef struct {
    int error;
    dg_surface draw, read;
    dg_context context;
    EGLDisplay egldisplay;
} dg_globals;

static pthread_key_t dg_globals_key;
static pthread_once_t create_global_key_once = PTHREAD_ONCE_INIT;

static void create_global_key()
{
    pthread_key_create(&dg_globals_key, NULL);
}

static inline dg_globals *dgGlobals()
{
    dg_globals* g;
    pthread_once(&create_global_key_once, create_global_key);
    if ((g = pthread_getspecific(dg_globals_key)) == NULL) {
        g = malloc(sizeof(dg_globals));
        g->error = DG_SUCCESS;
        g->draw = NULL;
        g->read = NULL;
        g->context = NULL;
        g->egldisplay = _egldisplay;
        pthread_setspecific(dg_globals_key, g);
    }
    return g;
}

static void dgSetError(int error)
{
    dgGlobals()->error = error;
}

int dgGetError()
{
    return dgGlobals()->error;
}

// Allows everything to break gracefully at version mismatch
static int version[] = {DG_VERSION_MAJOR, DG_VERSION_MINOR, 1};

const int* dgVersion()
{
    return version;
}

int dgValidVersion(int major, int minor)
{
    const int* version = dgVersion();
    return (version[0] == major) || (version[1] >= minor);
}

static EGLConfig find_eglconfig(int* attributes) {
    EGLBoolean result;
    EGLDisplay egldisplay = dgGlobals()->egldisplay;

    int len;
    for (len = 0; attributes[2*len] != 0; len++);

    EGLint attribute_list[2*len + 1];

    int i;
    int j = 0;
    for (i = 0; attributes[2*i] != 0; i++) {
        int value = attributes[2*i+1];
        switch(attributes[2*i]) {
            case DG_RED_CHANNEL:
                attribute_list[j++] = EGL_RED_SIZE;
                attribute_list[j++] = value;
                break;
            case DG_GREEN_CHANNEL:
                attribute_list[j++] = EGL_GREEN_SIZE;
                attribute_list[j++] = value;
                break;
            case DG_BLUE_CHANNEL:
                attribute_list[j++] = EGL_BLUE_SIZE;
                attribute_list[j++] = value;
                break;
            case DG_ALPHA_CHANNEL:
                attribute_list[j++] = EGL_ALPHA_SIZE;
                attribute_list[j++] = value;
                break;
            case DG_STENCIL_CHANNEL:
                attribute_list[j++] = EGL_STENCIL_SIZE;
                attribute_list[j++] = value;
                break;
        }
    }
    attribute_list[j++] = EGL_NONE;

    EGLConfig config;
    EGLint num_config;
    result = eglChooseConfig(egldisplay, attribute_list, &config, 1, &num_config);
    if (result == EGL_FALSE) {
        return NULL;
    }
    return config;
}

// Surface related functions
dg_surface dgCreateSurface(int width, int height, int* attributes)
{
    dg_globals* g = dgGlobals();
    dg_surface surface = malloc(sizeof(*surface));

    surface->eglconfig = find_eglconfig(attributes);

    EGLint pixel_format = EGL_PIXEL_FORMAT_ARGB_8888_BRCM;
    EGLint rt;
    eglGetConfigAttrib( g->egldisplay, surface->eglconfig, EGL_RENDERABLE_TYPE, &rt);

    if (rt & EGL_OPENGL_ES_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_GLES_BRCM;
        pixel_format |= EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM;
    }
    if (rt & EGL_OPENGL_ES2_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM;
        pixel_format |= EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM;
    }
    if (rt & EGL_OPENVG_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_VG_BRCM;
        pixel_format |= EGL_PIXEL_FORMAT_VG_IMAGE_BRCM;
    }
    if (rt & EGL_OPENGL_BIT) {
        pixel_format |= EGL_PIXEL_FORMAT_RENDER_GL_BRCM;
    }
    surface->data.global_image[0] = 0;
    surface->data.global_image[1] = 0;
    surface->data.global_image[2] = width;
    surface->data.global_image[3] = height;
    surface->data.global_image[4] = pixel_format;

    eglCreateGlobalImageBRCM(width, height, pixel_format, 0, width*4, surface->data.global_image);

    EGLint attrs[] = {
        EGL_VG_COLORSPACE, EGL_VG_COLORSPACE_sRGB,
        EGL_VG_ALPHA_FORMAT, pixel_format & EGL_PIXEL_FORMAT_ARGB_8888_PRE_BRCM ? EGL_VG_ALPHA_FORMAT_PRE : EGL_VG_ALPHA_FORMAT_NONPRE,
        EGL_NONE
    };
    surface->eglsurface = eglCreatePixmapSurface(
        g->egldisplay,
        surface->eglconfig,
        (EGLNativePixmapType)surface->data.global_image,
        attrs
    );
    if (surface->eglsurface == EGL_NO_SURFACE) {
        eglDestroyGlobalImageBRCM(surface->data.global_image);
        free(surface);
        dgSetError(DG_BAD_ALLOC);
        return 0;
    }
    surface->type = SHARED_SURFACE;
    return surface;
}

void dgSwapBuffers(dg_surface surface)
{
    if (surface == NULL) {
        dgSetError(DG_INVALID_ARGUMENT);
        return;
    }
    eglFlushBRCM();
    eglSwapBuffers(dgGlobals()->egldisplay, surface->eglsurface);
}

const char* dgSurfaceId(dg_surface surface)
{
    return surface->data.id;
}

void dgDestroySurface(dg_surface surface)
{
    switch (surface->type) {
        case SHARED_SURFACE: {
            eglDestroyGlobalImageBRCM(surface->data.global_image);
        } break;
        case NATIVE_SURFACE: {
            DISPMANX_UPDATE_HANDLE_T update;
            update = vc_dispmanx_update_start( 10 );
            vc_dispmanx_element_remove( update, surface->data.window.element );
            vc_dispmanx_update_submit_sync( update );
        } break;
    }
    eglDestroySurface(dgGlobals()->egldisplay, surface->eglsurface);
    free(surface);
}

// Context related functions
dg_context dgCreateContext(int api)
{
    dg_context context = malloc(sizeof(*context));
    context->api = api;
    context->eglconfig = NULL;
    context->eglcontext = EGL_NO_CONTEXT;
    return context;
}

static const EGLint gles2_context_attributes[] = 
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static EGLContext create_egl_context(dg_globals* g, int api, EGLConfig eglconfig)
{
    EGLBoolean e;
    const EGLint* attr = NULL;
    switch (api) {
        case DG_OPENGL_ES_2: {
            e = eglBindAPI(EGL_OPENGL_ES_API);
            attr = gles2_context_attributes;
        } break;
        case DG_OPENVG: {
            e = eglBindAPI(EGL_OPENVG_API);
        } break;
        default:
            e = EGL_FALSE;
    }
    if (e == EGL_FALSE) return EGL_NO_CONTEXT;
    return eglCreateContext(g->egldisplay, eglconfig, EGL_NO_CONTEXT, attr);
}

void dgMakeCurrent(dg_surface draw, dg_surface read, dg_context context)
{
    dg_globals* g = dgGlobals();
    if ((draw == NULL) && (read == NULL) && (context == NULL)) {
        eglMakeCurrent(g->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    } else {
        if ((draw == NULL) || (read == NULL) || (context == NULL)) {
            dgSetError(DG_INVALID_ARGUMENT);
            return;
        }

        if (context->eglcontext == EGL_NO_CONTEXT) {
            context->eglconfig = draw->eglconfig;
            context->eglcontext = create_egl_context(g, context->api, context->eglconfig);
            if (context->eglcontext == EGL_NO_CONTEXT) {
                //TODO: squeak heavy error.
                dgSetError(DG_BAD_ALLOC);
                return;
            }
        }
        eglMakeCurrent(g->egldisplay, draw->eglsurface, read->eglsurface, context->eglcontext);
    }
    g->draw = draw;
    g->read = read;
    g->context = context;

}

void dgGetCurrent(dg_surface* draw, dg_surface* read, dg_context* context)
{
    dg_globals* g = dgGlobals();
    *draw = g->draw;
    *read = g->read;
    *context = g->context;
}

void dgDestroyContext(dg_context context)
{
    if (context->eglcontext != EGL_NO_CONTEXT) {
        eglDestroyContext(dgGlobals()->egldisplay, context->eglcontext);
    }
    free(context);
}

// BRCM related extension
struct _dg_display_brcm {
    DISPMANX_DISPLAY_HANDLE_T   dmx_display;
    int                         resolution[2];
};

dg_display_brcm dgOpenDisplayBRCM(int id)
{
    DISPMANX_DISPLAY_HANDLE_T   dmx_display;

    dmx_display = vc_dispmanx_display_open(id);
    if ( dmx_display == 0 ) {
        dgSetError(DG_BAD_ALLOC);
        return 0;
    }

    dg_display_brcm display = malloc(sizeof(*display));
    display->dmx_display = dmx_display;
    return display;
}

void dgCloseDisplayBRCM(dg_display_brcm display)
{
    vc_dispmanx_display_close(display->dmx_display);
    display->dmx_display = 0;
    free(display);
}

const int* dgFullscreenResolutionBRCM(dg_display_brcm display)
{
    if (display == 0) {
        dgSetError(DG_INVALID_ARGUMENT);
        return 0;
    }
    DISPMANX_MODEINFO_T info;
    int ret;
    ret = vc_dispmanx_display_get_info(display->dmx_display, &info);
    if (ret != 0) {
        display->resolution[0] = 0;
        display->resolution[1] = 0;
        //TODO: dgSetError();
    } else {
        display->resolution[0] = info.width;
        display->resolution[1] = info.height;
    }
    return display->resolution;
}

dg_surface dgCreateFullscreenSurfaceBRCM(dg_display_brcm display, int x, int y, int width, int height, int layer, int* attributes)
{
    if (display == 0) {
        dgSetError(DG_INVALID_ARGUMENT);
        return 0;
    }
    VC_RECT_T                   dst_rect;
    VC_RECT_T                   src_rect;
    DISPMANX_UPDATE_HANDLE_T    update;
    DISPMANX_ELEMENT_HANDLE_T   element;

    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.width = width;
    dst_rect.height = height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = width << 16;
    src_rect.height = height << 16;

    update = vc_dispmanx_update_start( 0 );
    element = vc_dispmanx_element_add ( update, display->dmx_display,
                                        layer, &dst_rect, 0/*src*/,
                                        &src_rect, DISPMANX_PROTECTION_NONE,
                                        0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
    vc_dispmanx_update_submit_sync( update );

    dg_globals* g = dgGlobals();
    dg_surface surface = malloc(sizeof(*surface));
    surface->data.window.element = element;
    surface->data.window.width = width;
    surface->data.window.height = height;

    surface->eglconfig = find_eglconfig(attributes);
    surface->eglsurface = eglCreateWindowSurface( g->egldisplay, surface->eglconfig, &surface->data.window, NULL );

    if (surface->eglsurface == EGL_NO_SURFACE) {
        update = vc_dispmanx_update_start( 10 );
        vc_dispmanx_element_remove( update, surface->data.window.element );
        vc_dispmanx_update_submit_sync( update );
        free(surface);
        dgSetError(DG_BAD_ALLOC);
        return 0;
    }
    surface->type = NATIVE_SURFACE;
    return surface;
}

void dgUpdateFullscreenSurfaceBRCM(dg_surface surface, int x, int y, int width, int height, int layer)
{
}

// This belongs into different library, but would be inconvenient there. It's a hack.
//
#include <GLES2/gl2.h>

void glTextureSourceDG(long target, const char id[DG_SURFACE_ID_LENGTH])
{
    dg_globals* g = dgGlobals();
    int* gi = (int*)id;

    if (eglQueryGlobalImageBRCM(gi, gi+2)) {
        // this should set an opengl error instead.
        return;
    }
    
    EGLImageKHR image = (EGLImageKHR)eglCreateImageKHR(g->egldisplay, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)gi, NULL);
    if (image == EGL_NO_IMAGE_KHR) {
        // should too set an opengl error instead.
        return;
    }

    glEGLImageTargetTexture2DOES(target, image);

    eglDestroyImageKHR(g->egldisplay, image);
}
