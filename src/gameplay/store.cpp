#include "store.h"

#include "ui/ui.h"
#include "globals.h"

extern globals_t globals;

static store_t store;

const float store_t::TIME_BETWEEN_PURCHASES = 4.f;

extern float panel_left;

void add_store_credit(int amount) {
    store.store_credit += amount;
}

void update_store() {
    
    if (get_if_key_clicked_on("Buy")) {
		printf("buy button pressed\n");
        store.store_credit -= 5;
	}

    if (store.open) {
		set_ui_value(std::string("open_close_icon"), std::string("<<"));
	} else {
		set_ui_value(std::string("open_close_icon"), std::string(">>"));
	}
	set_ui_value(std::string("store_credit"), std::to_string(store.store_credit));

	if (get_if_key_clicked_on("open_close_section")) {
        store.open = !store.open;
		if (store.open) {
			panel_left = 0;
		} else {
			panel_left = -globals.window.window_width * 0.829f;
		}
		printf("open close section pressed\n");
	}
}