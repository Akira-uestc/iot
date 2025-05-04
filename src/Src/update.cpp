//
// Created by akira on 4/30/25.
//

const char* data_path = "/home/akira/data/data.json";
const char* control_path = "/home/akira/data/control.json";

#include "json_parser.h"

extern SystemData* data_container;
extern ControlData* control_container;

int sync_from_remote()
{
    JsonError jc = parse_control(control_path,control_container);
    JsonError jd = parse_data(data_path,data_container);
    return jc&&jd;
}
