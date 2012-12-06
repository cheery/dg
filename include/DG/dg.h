/* Direct graphics api for linux systems.
 * 2012-11-20
 * first draft for 1.0
 */
typedef struct dg_surface* dg_surface;
typedef struct dg_context* dg_context;

// only on broadcom
typedef struct dg_display_brcm* dg_display_brcm;

#define DG_VERSION_MAJOR 1
#define DG_VERSION_MINOR 0
const int* dgVersion();
int dgValidVersion(int major, int minor);

#define DG_RED_CHANNEL     1
#define DG_GREEN_CHANNEL   2
#define DG_BLUE_CHANNEL    3
#define DG_ALPHA_CHANNEL   4
#define DG_STENCIL_CHANNEL 5

#define DG_OPENVG          0x200
#define DG_OPENGL_ES_2     0x302

#define DG_SURFACE_ID_LENGTH 32

dg_surface dgCreateSurface(int width, int height, int* attributes);
void dgSwapBuffers(dg_surface surface);
const char* dgSurfaceId(dg_surface surface);
void dgDestroySurface(dg_surface surface);

dg_context dgCreateContext(int api);
void dgDestroyContext(dg_context context);
void dgMakeCurrent(dg_surface draw, dg_surface read, dg_context context);
void dgGetCurrent(dg_surface* draw, dg_surface* read, dg_context* context);

dg_display_brcm dgOpenDisplayBRCM(int id);
dg_surface dgCreateFullscreenSurfaceBRCM(dg_display_brcm display, int x, int y, int width, int height, int layer, int* attributes);
const int* dgFullscreenResolutionBRCM(dg_display_brcm display);
void dgCloseDisplayBRCM(dg_display_brcm display);

int dgGetError();

#define DG_SUCCESS 0
#define DG_INVALID_ARGUMENT 1
#define DG_BAD_ALLOC 2
