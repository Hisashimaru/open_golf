#pragma once

#include "mathf.h"

void app_create(int width, int height, const char *title);
bool app_process();
void app_swap_buffer();
void app_swap_interval(int interval);
void app_quit();
bool app_is_active();
void app_fullscreen(bool fullscreen);
bool app_is_fullscreen();
void app_set_resize_cb(void(*resize_cb)(int width, int height));
void app_set_size(int width, int height);
vec2 app_get_size();
void* app_get_context();
int app_get_fps();


// Time
void time_update();
float time_dt();
float time_now();
float time_last();