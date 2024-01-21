#include "renderer.h"

#include "window.h"
#include "quad.h"
#include "ui/ui.h"
#include "gameplay/bck.h"
#include "gameplay/gos.h"
#include "gameplay/preview_manager.h"
#include "utils/time.h"
#include "globals.h"
#include "utils/general.h"

#include <string>

extern globals_t globals;

extern PREVIEW_MODE preview_mode;

void render() {
	clear_window();

	if (globals.scene_manager.cur_level != LEVEL_1) {
		draw_from_ui_file_layouts();
		// autolayout_hierarchy();
		// end_imgui();
		render_ui();
		render_window();
		return;
	}

	// regular stuff
	draw_quad_renders();
	render_preview_mode();

	// ui_start_of_frame();		

	// draw_from_ui_file_layouts();

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


	// autolayout_hierarchy();
	// end_imgui();

	draw_from_ui_file_layouts();
	render_ui();
	render_window();
}

bool detect_gfx_error() {
	GLenum error = glGetError();
	if (error == GL_NO_ERROR) return false;
	return true;
}