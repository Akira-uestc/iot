#ifndef Coder_H
#define Coder_H

#include "status.h"

int code_light_groups_normal_mode(Light* light_struct);
void parse_light_groups(Light *light_struct);
int code_light_groups_smart_mode(Car_mgr* car_mgr, Light* light_struct);

#endif // Coder_H
