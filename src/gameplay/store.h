#pragma once

#include "utils/time.h"

struct store_t {
    int store_credit = 0;
    static const float TIME_BETWEEN_PURCHASES;
    time_count_t last_buy_time = TIME_BETWEEN_PURCHASES;
    bool open = true;
};

void add_store_credit(int amount);
void update_store();