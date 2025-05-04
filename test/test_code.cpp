//
// Created by akira on 4/30/25.
//

#include "../src/Inc/status.h"
#include <cstdio>
#include <cstdlib>

extern void parse_light_groups(Light *light_struct);
extern int code_light_groups_smart_mode(Car_mgr* car_mgr, Light* light_struct);

extern u_int16_t pwm_values[16];
extern u_int16_t origin_values[16];

void print_values(const char* name, u_int16_t values[16]) {
    printf("%s: [", name);
    for (int i = 0; i < 16; i++) {
        printf("%5u", values[i]);
        if (i < 15) printf(", ");
    }
    printf("]\n");
}

Car_mgr* create_test_car_mgr() {
    auto* mgr = (Car_mgr*)malloc(sizeof(Car_mgr));
    mgr->car_num = 2;

    auto* car1 = (Car*)malloc(sizeof(Car));
    car1->pos_x = 0.3;

    auto* car2 = (Car*)malloc(sizeof(Car));
    car2->pos_x = 0.7;

    car1->next = car2;
    car2->next = nullptr;

    mgr->head = car1;
    mgr->origin_head = car1;

    return mgr;
}

int main() {
    Car_mgr* car_mgr = create_test_car_mgr();
    Light light_struct;

    code_light_groups_smart_mode(car_mgr, &light_struct);
    print_values("origin_values", origin_values);

    // 解码为 PWM
    parse_light_groups(&light_struct);
    print_values("pwm_values", pwm_values);

    // 释放内存
    free(car_mgr->head->next);
    free(car_mgr->head);
    free(car_mgr);


    return 0;
}
