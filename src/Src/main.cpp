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
extern int configure_serial_recv(int fd);

int serial_fd_send = -1;
int serial_fd_recv = -1;

int main() {
    printf("Raspberry Pi client\n");

    // 打开串口0
    if ((serial_fd_send = open("/dev/serial0", O_RDWR | O_NOCTTY)) < 0) {
        perror("打开串口0失败\n");
        return -1;
    }
    if (configure_serial(serial_fd_send) < 0) {
        close(serial_fd_send);
        return -1;
    }

    // 打开串口1
    if ((serial_fd_recv= open("/dev/serial1", O_RDONLY | O_NOCTTY)) < 0) {
        perror("打开串口1失败\n");
        return -1;
    }
    if (configure_serial_recv(serial_fd_recv) < 0) {
        close(serial_fd_recv);
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
        
        // 通过serial0发送数据到stm32
        ssize_t written = write(serial_fd_send, light_struct, sizeof(Light));
        if (written < 0) {
            perror("写入失败");
            break;
        } else if (written != sizeof(Light)) {
            fprintf(stderr, "写入%zd字节,数据不完整\n", written);
        }
        printf("写入%zd字节\n",written);
        tcflush(serial_fd_send, TCOFLUSH);

        // 通过serial1从stm32读入数据
        ssize_t read_bytes = read(serial_fd_recv, recv_struct, sizeof(light));
        if (read_bytes < 0) {
            perror("读取错误");
            break;
        } else if (read_bytes == 0) {
            printf("等待数据中...\n");
            continue;
        } else if (read_bytes != sizeof(light)) {
            fprintf(stderr, "收到不完整数据 (%zd/%zu 字节)\n", 
                    read_bytes, sizeof(light));
            tcflush(serial_fd_recv, TCIFLUSH); // 清空缓冲区
            continue;
        }

        sleep(1);
    }
    close(serial_fd_send);
    close(serial_fd_recv);
    return 0;
}
