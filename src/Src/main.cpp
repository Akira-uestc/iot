/*
* Filename: main.c
 * Author: Akira
 * Description:
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include "json_parser.h"
#include "status.h"
#include <termios.h>
#include <pthread.h>

SystemData* data_container = (SystemData*)malloc(sizeof(SystemData));
ControlData* control_container = (ControlData*)malloc(sizeof(ControlData));
Car_mgr* car_mgr = (Car_mgr*)malloc(sizeof(Car_mgr));
Light* light_struct = (Light*)malloc(sizeof(Light));
Recv* recv_struct = (Recv*)malloc(sizeof(Recv));

int serial_fd = -1;

extern int sync_from_remote();
extern int configure_serial(int fd);
extern void* send_thread(void* arg);
extern void* recv_thread(void* arg);
extern void* car_update(void* arg);
extern void* collect_data(void* arg);

int main() {
    printf("Raspberry Pi client\n");

    // 打开串口0
    if ((serial_fd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY)) < 0) {
        perror("打开串口失败");
        return -1;
    }
    if (configure_serial(serial_fd) < 0) {
        close(serial_fd);
        return -1;
    }

    pthread_t send_tid, recv_tid, cam_tid, data_tid;
    pthread_create(&send_tid, NULL, send_thread, NULL);
    pthread_create(&recv_tid, NULL, recv_thread, NULL);
    pthread_create(&cam_tid, NULL, car_update, NULL);
    pthread_create(&data_tid, NULL, collect_data, NULL);

    pthread_join(send_tid, NULL);
    pthread_join(recv_tid, NULL);
    pthread_join(cam_tid, NULL);
    pthread_join(data_tid, NULL);

    close(serial_fd);
    return 0;
}
