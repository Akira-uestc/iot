//
// Created by akira on 4/30/25.
//

#ifndef STATUS_H
#define STATUS_H

#include <cstdint>
typedef struct car {
    double pos_x;
    double pos_y;
    double center;
    car* next;
    car* prev;
} Car;

typedef struct Car_mgr {
    int car_num;
    car* origin_head;
    car* head;
} Car_mgr;

typedef struct __attribute__((packed)) light {
    long long int light_group_0;
    long long int light_group_1;
    long long int light_group_2;
    long long int light_group_3;
} Light;

typedef struct recv {
    uint16_t light_sensor;
    uint16_t hall_sensor;
} Recv;

#endif //STATUS_H
