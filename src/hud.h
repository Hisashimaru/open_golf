#pragma once
#include "common.h"

void hud_init();
void hud_uninit();
void hud_draw();

void hud_change_club(ClubType type, int player_index);
void hud_hold_action(float value, int player_index);