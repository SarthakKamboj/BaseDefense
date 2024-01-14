#include "renderer.h"

#include "window.h"
#include "quad.h"
#include "ui/ui.h"
#include "gameobjects/bck.h"
#include "gameobjects/gos.h"

extern PREVIEW_MODE preview_mode;

void render() {
	clear_window();

	// regular stuff
	draw_quad_renders();

	start_of_frame();

	style_t panel_style;
	panel_style.vertical_align_val = ALIGN::CENTER;
	panel_style.horizontal_align_val = ALIGN::CENTER;
	push_style(panel_style);
	create_panel("main panel");
	pop_style();

	if (preview_mode == PREVIEW_MODE::PREVIEW_GUN) {
		create_text("preview gun mode");
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_ATTACHMENT) {
		create_text("preview attachment mode");
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE) {
		create_text("preview base mode");
	} else {
		create_text("unrecognized preview mode");
	}

	end_panel();

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