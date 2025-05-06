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

int get_current_hour() {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    return local->tm_hour;
}

// 单个LED的功率为50mW
void* collect_data(void* arg) {
    (void)arg;
    data_container->traffic[24] = {0};
    data_container->adjustment = 0;
    double power_comsumption[24] = {0};
    int last_hour = get_current_hour();
    int last_car_num = car_mgr->car_num;
    int init_hour = last_hour;

    while (1) {
        int current_hour = get_current_hour();
        for (int i = 0; i < 16; i++) {
            power_comsumption[current_hour] += (double)origin_values[i] / 65535.0 * 50.0 / 3600.0;
        }

        if (current_hour != last_hour) {
            int diff = car_mgr->car_num - last_car_num;
            data_container->traffic[last_hour] = diff;
            last_car_num = car_mgr->car_num;
            last_hour = current_hour;
        }

        data_container->uptime = current_hour - init_hour;
        memcpy(data_container->energy, power_comsumption, sizeof(data_container->energy));

        save_data(data_path, data_container);
        sleep(1);
    }
}
