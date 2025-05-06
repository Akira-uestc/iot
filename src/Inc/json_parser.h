//
// Created by akira on 5/2/25.
//

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

typedef struct {
    int mode;
    struct {
        int id;
        int brightness;
    } lights[16];
} ControlData;

typedef struct {
    int uptime;
    int adjustment;
    double energy[24];
    int traffic[24];
} SystemData;

typedef enum {
    JSON_OK,
    JSON_FILE_ERROR,
    JSON_PARSE_ERROR,
    JSON_INVALID_FORMAT
} JsonError;

JsonError parse_control(const char* filename, ControlData* out);

JsonError parse_data(const char* filename, SystemData* out);

JsonError save_control(const char* filename, const ControlData* data);

JsonError save_data(const char* filename, const SystemData* data);


#endif //JSON_PARSER_H
