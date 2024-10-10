// guest_inference_init.c
// Myungsuk (Jay) Moon

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(void) {
    // Set LD_LIBRARY_PATH
    if (setenv("LD_LIBRARY_PATH", "files/librknnrt/linux", 1) != 0) {
        perror("guest_inference_init: error: failed to set LD_LIBRARY_PATH");
        exit(errno);
    }

    // Execute the app with MobileNetV1
    execl("./files/guest_inference_minimal", "./files/guest_inference_minimal", "./files/models/mobilenetv1.rknn", NULL);

    // execl() has somehow failed
    perror("guest_inference_init: error");
    exit(errno);
}
