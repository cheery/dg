#include <DG/dg.h>
#include <error.h>
#include <stdio.h>

int main() {
    if (!dgValidVersion(1, 0)) { error(1, 0, "dg-1.0 unavailable"); }

    dg_display_brcm display = dgOpenDisplayBRCM(0);
    const int* wh = dgFullscreenResolutionBRCM(display);
    printf("%dx%d\n", wh[0], wh[1]);
    dgCloseDisplayBRCM(display);
}
