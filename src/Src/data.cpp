#include <math.h>
#include "json_parser.h"

extern SystemData* data_container;
extern const char* data_path;
extern u_int16_t origin_values[16];

// todo
double calculate_energy_comsuption() {
    save_data(data_path,data_container);
    return 0.0;
}