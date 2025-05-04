//
// Created by akira on 4/30/25.
//

#ifndef STATUS_H
#define STATUS_H

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

typedef struct light {
    long long int light_group_0;
    long long int light_group_1;
    long long int light_group_2;
    long long int light_group_3;
} Light;

typedef struct recv {
    int environment_brightness;
} Recv;

#endif //STATUS_H
