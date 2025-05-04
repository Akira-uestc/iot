/*
* Filename: main.c
 * Author: Akira
 * Description:
 */

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include "json_parser.h"
#include "coder.h"
#include "status.h"
#include <termios.h>

SystemData* data_container = (SystemData*)malloc(sizeof(SystemData));
ControlData* control_container = (ControlData*)malloc(sizeof(ControlData));
Car_mgr* car_mgr = (Car_mgr*)malloc(sizeof(Car_mgr));
Light* light_struct = (Light*)malloc(sizeof(Light));
Recv* recv_struct = (Recv*)malloc(sizeof(Recv));

extern int sync_from_remote();
extern int send_light_over_serial(Light* light);
extern int configure_serial(int fd);

int serial_fd = -1;

int main() {
    printf("Raspberry Pi client\n");

    // 打开串口0
    if ((serial_fd = open("/dev/serial0", O_RDWR | O_NOCTTY)) < 0) {
        perror("打开串口失败");
        return -1;
    }
    if (configure_serial(serial_fd) < 0) {
        close(serial_fd);
        return -1;
    }

    while (1) {
        // 解析control.json文件并设置灯光亮度数据
        sync_from_remote();
        if (control_container->mode == 1) {
            code_light_groups_normal_mode(light_struct);
        } else if (control_container->mode == 2) {
            code_light_groups_smart_mode(car_mgr, light_struct);
        } else {
            printf("Invalid mode\n");
        }
        
        // 发送灯光数据
        ssize_t written = write(serial_fd, light_struct, sizeof(Light));
        if (written < 0) {
            perror("写入失败");
            break;
        } else if (written != sizeof(Light)) {
            fprintf(stderr, "写入%zd字节, 数据不完整\n", written);
        }
        printf("写入%zd字节\n", written);
        tcflush(serial_fd, TCOFLUSH); // 清空发送缓冲区

        // 接收反馈数据
        ssize_t read_bytes = read(serial_fd, recv_struct, sizeof(Light));
        if (read_bytes < 0) {
            perror("读取失败");
            break;
        } else if (read_bytes == 0) {
            printf("等待数据中...\n");
            continue;
        } else if (read_bytes != sizeof(Light)) {
            fprintf(stderr, "收到不完整数据 (%zd/%zu 字节)\n", 
                    read_bytes, sizeof(Light));
            tcflush(serial_fd, TCIFLUSH); // 清空接收缓冲区
            continue;
        }

        sleep(1);
    }
    close(serial_fd);
    return 0;
}
