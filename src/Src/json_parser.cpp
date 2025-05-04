//
// Created by akira on 5/2/25.
//
#include "json_parser.h"
#include "cjson/cJSON.h"
#include <cstdio>
#include <cstdlib>

#define MAX_BUFFER_SIZE 4096

static char* read_file(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return nullptr;

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(fp);
        return nullptr;
    }

    fread(buffer, 1, length, fp);
    buffer[length] = '\0';

    fclose(fp);
    return buffer;
}

JsonError parse_control(const char* filename, ControlData* out) {
    char* json_str = read_file(filename);
    if (!json_str) return JSON_FILE_ERROR;

    cJSON* root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return JSON_PARSE_ERROR;

    // 解析模式
    cJSON* mode = cJSON_GetObjectItem(root, "currentMode");
    if (!mode || !cJSON_IsNumber(mode)) {
        cJSON_Delete(root);
        return JSON_INVALID_FORMAT;
    }
    out->mode = mode->valueint;

    // 解析灯光数据
    cJSON* lights = cJSON_GetObjectItem(root, "lights");
    if (!cJSON_IsArray(lights) || cJSON_GetArraySize(lights) != 16) {
        cJSON_Delete(root);
        return JSON_INVALID_FORMAT;
    }

    for (int i = 0; i < 16; i++) {
        cJSON* item = cJSON_GetArrayItem(lights, i);
        out->lights[i].id = cJSON_GetObjectItem(item, "id")->valueint;
        out->lights[i].brightness = cJSON_GetObjectItem(item, "brightness")->valueint;
    }

    cJSON_Delete(root);
    return JSON_OK;
}

JsonError parse_data(const char* filename, SystemData* out) {
    char* json_str = read_file(filename);
    if (!json_str) return JSON_FILE_ERROR;

    cJSON* root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return JSON_PARSE_ERROR;

    // 解析能耗数据
    cJSON* energy = cJSON_GetObjectItem(root, "energy");
    if (!cJSON_IsArray(energy) || cJSON_GetArraySize(energy) != 24) {
        cJSON_Delete(root);
        return JSON_INVALID_FORMAT;
    }

    for (int i = 0; i < 24; i++) {
        out->energy[i] = cJSON_GetArrayItem(energy, i)->valuedouble;
    }

    // 解析车流量数据
    cJSON* traffic = cJSON_GetObjectItem(root, "traffic");
    if (!cJSON_IsArray(traffic) || cJSON_GetArraySize(traffic) != 24) {
        cJSON_Delete(root);
        return JSON_INVALID_FORMAT;
    }

    for (int i = 0; i < 24; i++) {
        out->traffic[i] = cJSON_GetArrayItem(traffic, i)->valueint;
    }

    cJSON* uptime = cJSON_GetObjectItem(root, "uptime");
    if (!cJSON_IsArray(uptime) || cJSON_GetArraySize(uptime) != 1)
    {
        cJSON_Delete(root);
        return JSON_INVALID_FORMAT;
    }
    out->uptime = uptime->valueint;

    cJSON_Delete(root);
    return JSON_OK;
}

// 不使用
JsonError save_control(const char* filename, const ControlData* data) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "currentMode", data->mode);

    cJSON* lights = cJSON_CreateArray();
    for (int i = 0; i < 16; i++) {
        cJSON* light = cJSON_CreateObject();
        cJSON_AddNumberToObject(light, "id", data->lights[i].id);
        cJSON_AddNumberToObject(light, "brightness", data->lights[i].brightness);
        cJSON_AddItemToArray(lights, light);
    }
    cJSON_AddItemToObject(root, "lights", lights);

    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        free(json_str);
        return JSON_FILE_ERROR;
    }

    fputs(json_str, fp);
    fclose(fp);
    free(json_str);
    return JSON_OK;
}

JsonError save_data(const char* filename, const SystemData* data) {
    cJSON* root = cJSON_CreateObject();

    cJSON* energy = cJSON_CreateDoubleArray(data->energy, 24);
    cJSON_AddItemToObject(root, "energy", energy);

    cJSON* traffic = cJSON_CreateIntArray(data->traffic, 24);
    cJSON_AddItemToObject(root, "traffic", traffic);

    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        free(json_str);
        return JSON_FILE_ERROR;
    }

    fputs(json_str, fp);
    fclose(fp);
    free(json_str);
    return JSON_OK;
}