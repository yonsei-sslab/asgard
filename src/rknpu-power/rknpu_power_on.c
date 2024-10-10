// rknpu_power_on.c
// Myungsuk (Jay) Moon

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <drm/drm.h>

#define BUF_SIZE 32
#define RKNPU_ACTION 0x00
#define DRM_COMMAND_BASE 0x40
#define DRM_IOCTL_RKNPU_ACTION                                                 \
    DRM_IOWR(DRM_COMMAND_BASE + RKNPU_ACTION, struct rknpu_action)

/* action definitions */
enum e_rknpu_action {
    RKNPU_POWER_ON = 20,
};

/**
 * struct rknpu_task structure for action (GET, SET or ACT)
 *
 * @flags: flags for action
 * @value: GET or SET value
 *
 */
struct rknpu_action {
    __u32 flags;
    __u32 value;
};

int main(int argc, char **argv) {
    struct rknpu_action ioctl_struct;
    FILE *power;
    char buf[BUF_SIZE];
    int fd;
    int ret;

    fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(errno);
    }

    ioctl_struct.flags = RKNPU_POWER_ON;

    ret = ioctl(fd, DRM_IOCTL_RKNPU_ACTION, &ioctl_struct);
    /**
     * Do not handle errors. RKNPU device driver will always return EINVAL
     * regardless of the successful ioctl operation.
     */

    close(fd);

    /* Check device power status */
    power = fopen("/sys/kernel/debug/rknpu/power", "r");
    if (power == NULL) {
        perror("fopen");
        exit(errno);
    }

    while (fgets(buf, BUF_SIZE, power) != NULL) {
        printf("%s", buf);
    }

    fclose(power);

    return 0;
}
