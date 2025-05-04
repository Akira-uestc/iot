#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "status.h"

int configure_serial_recv(int fd) {
    struct termios options;
    if (tcgetattr(fd, &options) < 0) {
        perror("获取配置失败");
        return -1;
    }

    cfmakeraw(&options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag |= (CLOCAL | CREAD); // 启用接收
    options.c_cc[VMIN] = sizeof(Light); // 等待足够字节
    options.c_cc[VTIME] = 10; // 1秒超时

    if (tcsetattr(fd, TCSANOW, &options) < 0) {
        perror("配置失败");
        return -1;
    }
    return 0;
}