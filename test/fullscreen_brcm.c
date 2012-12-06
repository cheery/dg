#include <DG/dg.h>
#include <GLES2/gl2.h>
#include <unistd.h>
#include <error.h>

int main() {
    int e;
    if (!dgValidVersion(1, 0)) { error(1, 0, "dg-1.0 unavailable"); }

    dg_display_brcm dpy = dgOpenDisplayBRCM(0);
    const int* wh = dgFullscreenResolutionBRCM(dpy);

    if ((e = dgGetError()) != DG_SUCCESS) {
        error(1, 0, "dg error=%d on line: %d\n", e, __LINE__);
    }

    int attrs[] = {
        DG_RED_CHANNEL,   8,
        DG_GREEN_CHANNEL, 8,
        DG_BLUE_CHANNEL,  8,
        DG_ALPHA_CHANNEL, 8,
        0
    };

    dg_surface surface = dgCreateFullscreenSurfaceBRCM(dpy, 0, 0, wh[0], wh[1], 0, attrs);
    if ((e = dgGetError()) != DG_SUCCESS) {
        error(1, 0, "dg error=%d after create surface\n", e);
    }

    dg_context context = dgCreateContext(DG_OPENGL_ES_2);
    if ((e = dgGetError()) != DG_SUCCESS) {
        error(1, 0, "dg error=%d after create context\n", e);
    }


    dgMakeCurrent(surface, surface, context);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    dgSwapBuffers(surface);

    sleep(1);

    return 0;
}
