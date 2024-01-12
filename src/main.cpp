#include <iostream>

#include "update.h"
#include "gfx/renderer.h"
#include "utils/time.h"
#include "constants.h"
#include "init.h"
#include "utils/io.h"

#include "globals.h"

#define WANT_EXTRA_INFO 1

globals_t globals;

bool level_finished = false;

int main(int argc, char *argv[])
{	 
	bool running_in_vs = false;
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			if (strcmp("RUN_FROM_IDE", argv[i]) == 0) {
				running_in_vs = true;
			}
		}
	}

	set_running_in_visual_studio(running_in_vs);

#if WANT_EXTRA_INFO
	std::cout << "running_in_vs: " << running_in_vs << std::endl;

	char buffer[256]{};
	get_resources_folder_path(buffer);
	char resource_path_info[512]{};
	sprintf(resource_path_info, "resource path: %s\n", buffer);
	game_info_log(resource_path_info);
#endif

	init();	

	game_info_log("finished init");

	while (globals.running)
    {
		game_timer_t frame_timer;
		start_timer(frame_timer);

		process_input();

		globals.running = !globals.window.user_input.quit;

		update();
		render();

		end_timer(frame_timer);

		game::time_t::delta_time = fmin(frame_timer.elapsed_time_sec, 1 / 60.f);
		game::time_t::cur_time += game::time_t::delta_time;	

		if (globals.window.user_input.p_pressed) {
			std::cout << "fps: " << 1 / game::time_t::delta_time << std::endl;
		}

		detect_window_error();

		globals.window.resized = false;
    }

	return EXIT_SUCCESS;
}