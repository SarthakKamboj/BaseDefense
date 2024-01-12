#include "renderer.h"

#include "window.h"
#include "quad.h"
#include "ui/ui.h"

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

	create_text("Base Defense!", TEXT_SIZE::TITLE);

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