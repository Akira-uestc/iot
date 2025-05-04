//
// Created by akira on 4/30/25.
//

#include <cstdio>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "../Inc/status.h"
#include <cstring>

// 单次发送，不使用
int send_light_over_serial(Light* light) {
    const char* device = "/dev/serial0";
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("open serial port failed");
        return -1;
    }

    termios options{};
    tcgetattr(fd, &options);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= (CLOCAL | CREAD);

    tcsetattr(fd, TCSANOW, &options);

    unsigned char buffer[sizeof(Light)];
    memcpy(buffer, light, sizeof(Light));

    int written = write(fd, buffer, sizeof(Light));
    if (written < 0) {
        perror("write failed");
        close(fd);
        return -1;
    }

    printf("Sent %d bytes over serial\n", written);
    close(fd);
    return 0;
}

int configure_serial(int fd) {
    termios options{};
    if (tcgetattr(fd, &options)) {
        perror("获取串口属性失败");
        return -1;
    }

    cfmakeraw(&options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~CSTOPB;  // 1位停止位
    options.c_cc[VMIN] = 0;     // 非阻塞读取
    options.c_cc[VTIME] = 10;   // 1秒超时

    if (tcsetattr(fd, TCSANOW, &options)) {
        perror("配置串口失败");
        return -1;
    }
    return 0;
}