#pragma once

#include "utils/time.h"

enum ITEM {
    ITEM_NONE = 0,
    ITEM_BASE,
    ITEM_BASE_EXT,
    ITEM_GUN
};

struct store_item_t {
    ITEM item = ITEM_NONE;
    int cost = 0;
};

struct store_t {
    int store_credit = 0;
    static const float TIME_BETWEEN_PURCHASES;
    time_count_t last_buy_time = TIME_BETWEEN_PURCHASES;
    bool open = true;
    ITEM selected_item = ITEM_NONE;
};

void add_store_credit(int amount);
void update_store();

struct inventory_t {
    int num_bases = 1;
    int num_base_exts = 1;
    int num_guns = 1;
};