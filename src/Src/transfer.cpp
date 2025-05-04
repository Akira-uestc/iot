//
// Created by akira on 4/30/25.
//

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include "../Inc/status.h"
#include "coder.h"
#include <cstring>
#include <pthread.h>
#include "json_parser.h"

extern int sync_from_remote();

extern ControlData* control_container;
extern int serial_fd;
extern Recv* recv_struct;
extern Car_mgr* car_mgr;
extern Light* light_struct;

int configure_serial(int fd) {
    struct termios options{};
    if (tcgetattr(fd, &options) < 0) {
        perror("获取串口属性失败");
        return -1;
    }

    cfmakeraw(&options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag |= (CLOCAL | CREAD); // 启用接收
    options.c_cflag &= ~CSTOPB;          // 1位停止位
    options.c_cc[VMIN] = 0;  // 接收最小字节数
    options.c_cc[VTIME] = 10;            // 超时 1 秒（单位0.1s）

    if (tcsetattr(fd, TCSANOW, &options) < 0) {
        perror("配置串口失败");
        return -1;
    }
    return 0;
}

void* recv_thread(void* arg) {
    (void)arg;
    while (1) {
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(serial_fd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int retval = select(serial_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (retval == -1) {
            perror("[recv] select error");
            continue;
        } else if (retval == 0) {
            continue;
        }

        ssize_t read_bytes = read(serial_fd, recv_struct, sizeof(Recv));
        if (read_bytes < 0) {
            if (errno == EINTR) continue;
            perror("[recv] read error");
            continue;
        } else if (read_bytes != sizeof(Recv)) {
            fprintf(stderr, "[recv] 不完整数据 (%zd/%zu 字节)\n",
                    read_bytes, sizeof(Recv));
            tcflush(serial_fd, TCIFLUSH);
            continue;
        }

        printf("[recv] 收到%zd字节反馈数据\n", read_bytes);
    }
    return NULL;
}

void* send_thread(void* arg) {
    (void)arg;
    while (1) {
        sync_from_remote();

        if (control_container->mode == 1) {
            code_light_groups_normal_mode(light_struct);
        } else if (control_container->mode == 2) {
            code_light_groups_smart_mode(car_mgr, light_struct);
        } else {
            printf("[send] Invalid mode\n");
            sleep(1);
            continue;
        }

        ssize_t written = write(serial_fd, light_struct, sizeof(Light));
        if (written < 0) {
            perror("[send] 写入失败");
        } else {
            printf("[send] 写入%zd字节\n", written);
        }

        tcflush(serial_fd, TCOFLUSH);
        sleep(1); // 控制发送间隔
    }
    return NULL;
}
