#include <cstdio>
#include <cstring>
#include <math.h>
#include "json_parser.h"
#include "status.h"
#include <time.h>
#include <unistd.h>

extern SystemData* data_container;
extern ControlData* control_container;
extern const char* data_path;
extern u_int16_t origin_values[16];
extern Car_mgr* car_mgr;
extern Recv* recv_struct;

int get_current_hour() {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    return local->tm_hour;
}

// 单个LED的功率为50mW
void* collect_data(void* arg) {
    (void)arg;
    data_container->traffic[24] = {0};
    double power_comsumption[24] = {0};
    memcpy(power_comsumption, data_container->energy, sizeof(data_container->energy));

    int last_hour = get_current_hour();
    int last_car_num = car_mgr->car_num;
    int init_hour = last_hour;

    uint16_t prev_hall_sensor = recv_struct->hall_sensor;  // 初始化上一个 hall 值

    while (1) {
        // 计算光照调整值
        data_container->adjustment = (double)recv_struct->light_sensor / 4096.0 * 100;
        // printf("adjustment: %d\n", data_container->adjustment);

        int current_hour = get_current_hour();

        // 功耗统计
        for (int i = 0; i < 16; i++) {
            power_comsumption[current_hour] += (double)origin_values[i] / 65535.0 * 50.0 / 3600.0;
        }

        // 检测 hall_sensor 上升沿
        if (prev_hall_sensor == 0 && recv_struct->hall_sensor == 1) {
            car_mgr->car_num++;
            printf("car passed, car_num: %d\n", car_mgr->car_num);
        }
        prev_hall_sensor = recv_struct->hall_sensor;

        // 整点更新车流量
        if (current_hour != last_hour) {
            int diff = car_mgr->car_num - last_car_num;
            data_container->traffic[last_hour] = diff;
            last_car_num = car_mgr->car_num;
            last_hour = current_hour;
        }

        data_container->uptime = current_hour - init_hour;
        memcpy(data_container->energy, power_comsumption, sizeof(data_container->energy));

        // 调试输出，需要时再取消注释
        // printf("light_sensor: %d\n", recv_struct->light_sensor);
        // printf("hall_sensor: %d\n", recv_struct->hall_sensor);

        save_data(data_path, data_container);
    }
}

