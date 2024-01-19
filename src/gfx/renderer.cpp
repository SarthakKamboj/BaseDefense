#include "renderer.h"

#include "window.h"
#include "quad.h"
#include "ui/ui.h"
#include "gameplay/bck.h"
#include "gameplay/gos.h"
#include "utils/time.h"
#include "globals.h"
#include "utils/general.h"

#include <string>

extern globals_t globals;

extern PREVIEW_MODE preview_mode;

void render() {
	clear_window();

	if (globals.scene_manager.cur_level == TEST_UI_LEVEL) {
		// ui_start_of_frame();

		// if (panel_left == 0) {
		// 	set_ui_value(std::string("open_close_icon"), std::string("<<"));
		// } else {
		// 	set_ui_value(std::string("open_close_icon"), std::string(">>"));
		// }
		// draw_from_ui_file_layout();

		// if (get_if_key_clicked_on("Buy")) {
		// 	printf("buy button pressed\n");
		// }

		// if (get_if_key_clicked_on("open_close_section")) {
		// 	if (panel_left == 0) {
		// 		panel_left = -globals.window.window_width * 0.829f;
		// 	} else {
		// 		panel_left = 0;
		// 	}
		// 	printf("open close section pressed\n");
		// }

		autolayout_hierarchy();
		end_imgui();
		render_ui();
		render_window();
		return;
	}

	// regular stuff
	draw_quad_renders();

	// ui_start_of_frame();	

	if (preview_mode == PREVIEW_MODE::PREVIEW_GUN) {
		set_ui_value(std::string("preview_mode"), std::string("preview gun mode"));
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
		set_ui_value(std::string("preview_mode"), std::string("preview base ext mode"));
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE) {
		set_ui_value(std::string("preview_mode"), std::string("preview base mode"));
	} else {
		set_ui_value(std::string("preview_mode"), std::string("unrecognized preview mode"));
	}

	draw_from_ui_file_layouts();

	// style_t panel_style;
	// panel_style.vertical_align_val = ALIGN::START;
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