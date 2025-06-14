//
// Created by akira on 4/30/25.
//

#include "../Inc/status.h"
#include "../Inc/json_parser.h"
#include <cmath>
#include <sys/types.h>
#include <stdio.h>

u_int16_t pwm_values[16] = {0};
u_int16_t origin_values[16] = {0};
extern ControlData* control_container;
extern SystemData* data_container;
extern Recv* recv_struct;

#define LIGHT_RADIUS 0.2  // 灯影响范围（单位灯区间为1.0）
#define GAUSSIAN_SIGMA 0.1  // 标准差

double gaussian(double x, double mu, double sigma) {
    double diff = x - mu;
    return exp(- (diff * diff) / (2 * sigma * sigma));
}

int code_light_groups_smart_mode(Car_mgr* car_mgr, Light* light_struct) {
    Car* current = NULL;

    for (int x = 0; x < 16; x++) {
        double light_pos = (double)x / 16.0;
        double brightness = 0.0;

        current = car_mgr->origin_head;
        while (current != NULL) {
            double car_pos = current->center;
            double distance = fabs(light_pos - car_pos);

            if (distance <= LIGHT_RADIUS) {
                // 使用高斯函数模拟灯光影响
                brightness += gaussian(light_pos, car_pos, GAUSSIAN_SIGMA);
            }

            current = current->next;
        }

        // 归一化亮度
        if (brightness > 1.0) brightness = 1.0;

        origin_values[x] = (uint16_t)(brightness * data_container->adjustment * 65535.0 / 100.0);
    }

    // 灯光打包
    long long int groups[4] = {0};
    for (int i = 0; i < 4; i++) {
        long long int packed = 0;
        for (int j = 0; j < 4; j++) {
            int index = i * 4 + j;
            packed |= ((long long int)origin_values[index] & 0xFFFF) << (j * 16);
        }
        groups[i] = packed;
    }

    light_struct->light_group_0 = groups[0];
    light_struct->light_group_1 = groups[1];
    light_struct->light_group_2 = groups[2];
    light_struct->light_group_3 = groups[3];

    return 0;
}

int code_light_groups_normal_mode(Light* light_struct) {
    long long int groups[4] = {0};

    for (int i = 0; i <16; i++) {
        double percentage = (double)control_container->lights[i].brightness / 100.0;
        origin_values[i] = percentage*65535;
    }

    for (int i = 0; i < 4; i++) {
        long long int packed = 0;
        for (int j = 0; j < 4; j++) {
            int index = i * 4 + j;
            packed |= ((long long int)origin_values[index] & 0xFFFF) << (j * 16);
        }
        groups[i] = packed;
    }

    light_struct->light_group_0 = groups[0];
    light_struct->light_group_1 = groups[1];
    light_struct->light_group_2 = groups[2];
    light_struct->light_group_3 = groups[3];

    return 0;
}

void parse_light_groups(Light *light_struct) {
    long long int groups[4] = {
        light_struct->light_group_0,
        light_struct->light_group_1,
        light_struct->light_group_2,
        light_struct->light_group_3
    };

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            pwm_values[i * 4 + j] = (groups[i] >> (j * 16)) & 0xFFFF;
        }
    }
}
