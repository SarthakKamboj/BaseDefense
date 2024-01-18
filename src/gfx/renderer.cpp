#include "renderer.h"

#include "window.h"
#include "quad.h"
#include "ui/ui.h"
#include "gameobjects/bck.h"
#include "gameobjects/gos.h"
#include "utils/time.h"
#include "globals.h"
#include "utils/general.h"

extern globals_t globals;

extern PREVIEW_MODE preview_mode;

void render() {
	clear_window();

	if (globals.scene_manager.cur_level == TEST_UI_LEVEL) {
		start_of_frame();
		draw_from_ui_file_layout();

		if (get_if_key_clicked_on("Buy")) {
			printf("buy button pressed\n");
		}

		autolayout_hierarchy();
		end_imgui();
		render_ui();
		render_window();
		return;
	}

	// regular stuff
	draw_quad_renders();

	start_of_frame();

	char buffer[32]{};
	int_to_string((int)game::time_t::cur_time, buffer);
	set_ui_value(std::string("score"), std::string(buffer));

	std::string preview_mode_str;
	if (preview_mode == PREVIEW_MODE::PREVIEW_GUN) {
		preview_mode_str = "preview gun mode";
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
		preview_mode_str = "preview base ext mode";
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE) {
		preview_mode_str = "preview base mode";
	} else {
		preview_mode_str = "unrecognized preview mode";
	}
	set_ui_value(std::string("preview_mode"), preview_mode_str);

	draw_from_ui_file_layout();

	// style_t panel_style;
	// panel_style.vertical_align_val = ALIGN::CENTER;
	// panel_style.horizontal_align_val = ALIGN::CENTER;
	// push_style(panel_style);
	// create_panel("main panel");
	// pop_style();

	// if (preview_mode == PREVIEW_MODE::PREVIEW_GUN) {
	// 	create_text("preview gun mode");
	// } else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
	// 	create_text("preview base ext mode");
	// } else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE) {
	// 	create_text("preview base mode");
	// } else {
	// 	create_text("unrecognized preview mode");
	// }

	// end_panel();

	autolayout_hierarchy();
	end_imgui();

	render_ui();

	render_window();
}

bool detect_gfx_error() {
	GLenum error = glGetError();
	if (error == GL_NO_ERROR) return false;
	return true;
}