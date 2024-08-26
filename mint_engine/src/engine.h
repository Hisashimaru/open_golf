#pragma once

void init_engine();
void uninit_engine();
void start_engine(void(*update_cb)(), void(*draw_cb)());