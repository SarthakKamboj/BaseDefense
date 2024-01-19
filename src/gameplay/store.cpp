#include "store.h"

#include "ui/ui.h"
#include "globals.h"
#include "utils/general.h"

extern globals_t globals;

static store_t store;
inventory_t inventory;

const float store_t::TIME_BETWEEN_PURCHASES = 4.f;

extern float panel_left;

static store_item_t store_items[3] = {
    store_item_t {
        ITEM_BASE,
        10
    }, store_item_t {
        ITEM_BASE_EXT,
        5
    }, store_item_t {
        ITEM_GUN,
        2
    }
};

void add_store_credit(int amount) {
    store.store_credit += amount;
}

void update_store() {
    
    if (get_if_key_clicked_on("base_container")) {
        store.selected_item = ITEM_BASE;
    } else if (get_if_key_clicked_on("base_ext_container")) {
        store.selected_item = ITEM_BASE_EXT;
    } else if (get_if_key_clicked_on("gun_container")) {
        store.selected_item = ITEM_GUN;
    }

    glm::vec3 selected_color = create_color(220,115,0);
    switch (store.selected_item) {
        case ITEM_BASE: {
            set_background_color_override("base_container", selected_color);
            break;
        }
        case ITEM_BASE_EXT: {
            set_background_color_override("base_ext_container", selected_color);
            break;
        }
        case ITEM_GUN: {
            set_background_color_override("gun_container", selected_color);
            break;
        }
    }
    
    if (get_if_key_clicked_on("Buy") && store.selected_item != ITEM_NONE && store.store_credit >= store_items[store.selected_item-1].cost) {
        store.store_credit -= store_items[store.selected_item-1].cost;
        switch (store.selected_item) {
            case ITEM_BASE: {
                inventory.num_bases += 1;
                break;
            }
            case ITEM_BASE_EXT: {
                inventory.num_base_exts += 1;
                break;
            }
            case ITEM_GUN: {
                inventory.num_guns += 1;
                break;
            }
        }
	}

    if (store.open) {
		set_ui_value(std::string("open_close_icon"), std::string("<<"));
	} else {
		set_ui_value(std::string("open_close_icon"), std::string(">>"));
	}
	set_ui_value(std::string("store_credit"), std::to_string(store.store_credit));

    set_ui_value(std::string("base_cost"), std::to_string(store_items[ITEM_BASE-1].cost));
    set_ui_value(std::string("base_ext_cost"), std::to_string(store_items[ITEM_BASE_EXT-1].cost));
    set_ui_value(std::string("gun_cost"), std::to_string(store_items[ITEM_GUN-1].cost));

    set_ui_value(std::string("num_guns"), std::to_string(inventory.num_guns));
    set_ui_value(std::string("num_bases"), std::to_string(inventory.num_bases));
    set_ui_value(std::string("num_base_exts"), std::to_string(inventory.num_base_exts));

	if (get_if_key_clicked_on("open_close_section")) {
        store.open = !store.open;
		if (store.open) {
            store.selected_item = ITEM_NONE;
			panel_left = 0;
		} else {
            store.selected_item = ITEM_NONE;
			panel_left = -globals.window.window_width * 0.829f;
		}
	}
}