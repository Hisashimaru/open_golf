#pragma once
#include "common.h"

void game_init();
void game_uninit();
void game_update();
void game_draw();
void game_cupin();
void game_change_map(const char *filename);
void game_next_map();
bool game_has_next_map();
void game_change_mode(GameMode mode);