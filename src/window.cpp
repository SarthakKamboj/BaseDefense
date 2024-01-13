#include "window.h"

#include <string>

#include "constants.h"
#include "globals.h"

#include "glad/glad.h"

extern globals_t globals;

static aspect_ratio_t aspect_ratios[ASPECT_RATIO::NUM_RATIOS] = {
	aspect_ratio_t {
		ASPECT_RATIO::A_1600x900,
		"1600 x 900",
		1600,
		900
	},
	aspect_ratio_t {
		ASPECT_RATIO::A_1600x1000,
		"1600 x 1000",
		1600,
		1000
	},
	aspect_ratio_t {
		ASPECT_RATIO::A_1600x1200,
		"1600 x 1200",
		1600,
		1200
	}
};

int get_mode_index(float w, float h) {
	int num_display_modes = SDL_GetNumDisplayModes(0);
	float ratio = w / h;
	for (int i = 0; i < num_display_modes; i++) {
		SDL_DisplayMode display_mode{};
		if (SDL_GetDisplayMode(0, i, &display_mode) != 0) {
			SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
		}
		// SDL_Log("Mode %i\tname: %s\t%i x %i and ratio is %f",
		// 		i,
		// 		SDL_GetPixelFormatName(display_mode.format),
		// 		display_mode.w, display_mode.h, static_cast<float>(display_mode.w) / display_mode.h);
		
		float display_ratio = static_cast<float>(display_mode.w) / display_mode.h;
		if (floor(display_ratio * 1000) == floor(ratio * 1000)) {
			return i;
		}
	}
	return -1;
}

/// <summary>
/// This function initalizes SDL to OpenGL Version 4.1, 24-bit depth buffer, height of WINDOW_HEIGHT, 
/// width of WINDOW_WIDTH, blending enabled, and mirrored texture wrapping.
/// </summary>
/// <returns>The SDL window that gets created.</returns>
/// <see>See constants.h for WINDOW_HEIGHT and WINDOW_WIDTH</see>
void init_window() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		detect_window_error();
        game_throw_error();
        return;
	}

    window_t& window = globals.window;

    // setting sdl attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1); 

	globals.window.is_full_screen = false;
	if (globals.window.is_full_screen) {
		globals.window.sdl_window = SDL_CreateWindow("Base Defense", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, -1, -1, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN);
	}
	else {
        float width = STARTING_WINDOW_WIDTH;
        float height = width * (900.f / 1600.f);
		globals.window.sdl_window = SDL_CreateWindow("Base Defense", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	}

    // if window is null, then window creation that an issue
	if (globals.window.sdl_window == NULL) {
        detect_window_error();
        game_throw_error();
        return;
	}

    SDL_Window* sdl_window = window.sdl_window;

    for (int i = 0; i < ASPECT_RATIO::NUM_RATIOS; i++) {
		aspect_ratio_t& aspect_ratio = aspect_ratios[i];
		aspect_ratio.mode_index = get_mode_index(aspect_ratio.width, aspect_ratio.height);

		if (aspect_ratio.ratio == ASPECT_RATIO::A_1600x900) {
			SDL_DisplayMode mode{};
			SDL_GetDisplayMode(0, aspect_ratio.mode_index, &mode);
			int success = SDL_SetWindowDisplayMode(sdl_window, &mode);
			if (success != 0) {
                char error[256]{};
                sprintf(error, "ran into an issue with setting display mode index %i\n", aspect_ratio.mode_index);
				game_error_log(error);
				detect_window_error();
                game_throw_error();
			}
		}
	}

	int window_width, window_height;
	SDL_GetWindowSize(sdl_window, &window_width, &window_height);
	window.window_width = window_width;
	window.window_height = window_height;

	SDL_GLContext context = SDL_GL_CreateContext(sdl_window);
    // load opengl functions
	gladLoadGLLoader(SDL_GL_GetProcAddress);

	SDL_GL_MakeCurrent(sdl_window, context);

    // setting game default texture parameters and blending settings
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);
}

void clear_window() {
    glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_window() {
    SDL_GL_SwapWindow(globals.window.sdl_window);
}

void init_controller() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            globals.window.user_input.game_controller = SDL_GameControllerOpen(i);
            game_info_log("found game controller joystick");
        }
    }
}

void process_input() {
    window_t& window = globals.window;
    user_input_t& user_input = window.user_input;

    user_input.controller_state_changed = false;

    SDL_GetMouseState(&user_input.mouse_x, &user_input.mouse_y);
    user_input.mouse_y = globals.window.window_height - user_input.mouse_y;

    user_input.some_key_pressed = false;
    user_input.w_pressed = false;
    user_input.a_pressed = false;
    user_input.s_pressed = false;
    user_input.d_pressed = false;
    user_input.p_pressed = false;
    user_input.l_pressed = false;
    user_input.space_pressed = false;
    user_input.enter_pressed = false;

    user_input.controller_a_pressed = false;
    user_input.controller_y_pressed = false;
    user_input.controller_x_pressed = false;
    user_input.controller_b_pressed = false;
    user_input.controller_start_pressed = false;

    user_input.quit = false;
    user_input.left_clicked = false;
    user_input.right_clicked = false;

    user_input.controller_x_axis = 0;
    user_input.controller_y_axis = 0;

    if (user_input.game_controller) {
        user_input.controller_x_axis = SDL_GameControllerGetAxis(user_input.game_controller, SDL_CONTROLLER_AXIS_LEFTX) / 32768.f;
        user_input.controller_y_axis = SDL_GameControllerGetAxis(user_input.game_controller, SDL_CONTROLLER_AXIS_LEFTY) / -32768.f;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                user_input.quit = true;
                break;
            }
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_MOVED: {
                        int x = event.window.data1;
                        int y = event.window.data2;
                        break;
                    }
                    // seems like this is called 2x on resizes
                    case SDL_WINDOWEVENT_RESIZED: {
                        break;
                    }
                    // seems like this is called 1x on resizes
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        printf("prev window size: %f, %f\n", window.window_width, window.window_height);
                        window.window_width = event.window.data1;
                        window.window_height = event.window.data2;
                        printf("new window size: %f, %f\n", window.window_width, window.window_height);
                        window.resized = true;
                        glViewport(0, 0, window.window_width, window.window_height);
                        break;
                    }
                    default: break;
                }
                break;
            }
            case SDL_CONTROLLERDEVICEADDED: {
                if (!user_input.game_controller) {
                    user_input.controller_state_changed = true;
                    init_controller();
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                if (user_input.game_controller) {
                    SDL_Joystick* controller_joystick = SDL_GameControllerGetJoystick(user_input.game_controller);
                    if (event.cdevice.which == SDL_JoystickInstanceID(controller_joystick)) {
                        user_input.controller_state_changed = true;
                        user_input.game_controller = NULL;
                        user_input.controller_a_down = false;
                        user_input.controller_a_pressed = false;
                        user_input.controller_y_down = false;
                        user_input.controller_y_pressed = false;
                        user_input.controller_x_down = false;
                        user_input.controller_x_pressed = false;
                        user_input.controller_b_down = false;
                        user_input.controller_b_pressed = false;
                        user_input.controller_start_down = false;
                        user_input.controller_start_pressed = false;
                        user_input.controller_x_axis = 0;
                        user_input.controller_y_axis = 0;
                        init_controller();
                    }
                }
                break;
            }
            case SDL_CONTROLLERBUTTONUP: {
                if (user_input.game_controller) {
                    SDL_Joystick* controller_joystick = SDL_GameControllerGetJoystick(user_input.game_controller);
                    if (event.cdevice.which == SDL_JoystickInstanceID(controller_joystick)) {
                        printf("button: %i\n", (int)event.cbutton.button);
                        switch (event.cbutton.button) {
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: {
                                user_input.controller_a_down = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: {
                                user_input.controller_y_down = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: {
                                user_input.controller_b_down = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: {
                                user_input.controller_x_down = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: {
                                user_input.controller_start_down = false;
                                break;
                            }
                            default: break;
                        }
                    }
                }
                break;	
            }

            case SDL_CONTROLLERBUTTONDOWN: {
                if (user_input.game_controller) {
                    user_input.some_key_pressed = true;
                    SDL_Joystick* controller_joystick = SDL_GameControllerGetJoystick(user_input.game_controller);
                    if (event.cdevice.which == SDL_JoystickInstanceID(controller_joystick)) {
                        switch (event.cbutton.button) {
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: {
                                user_input.controller_a_pressed = true;
                                user_input.controller_a_down = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: {
                                user_input.controller_y_pressed = true;
                                user_input.controller_y_down = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: {
                                user_input.controller_b_pressed = true;
                                user_input.controller_b_down = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: {
                                user_input.controller_x_pressed = true;
                                user_input.controller_x_down = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: {
                                user_input.controller_start_down = true;
                                user_input.controller_start_pressed = true;
                                break;
                            }
                            default: break;
                        }
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                user_input.right_clicked = (event.button.button == SDL_BUTTON_RIGHT);
                user_input.left_clicked = (event.button.button == SDL_BUTTON_LEFT);	
            }
            case SDL_KEYUP: {
                switch (event.key.keysym.sym) {	
                    case SDLK_w: {
                        user_input.w_down = false;
                        break;
                    }
                    case SDLK_a: {
                        user_input.a_down = false;
                        break;
                    }
                    case SDLK_s: {
                        user_input.s_down = false;
                        break;
                    }
                    case SDLK_d: {
                        user_input.d_down = false;
                        break;
                    }
                    case SDLK_p: {
                        user_input.p_down = false;
                        break;
                    }
                    case SDLK_l: {
                        user_input.l_down = false;
                        break;
                    }
                    case SDLK_RETURN: {
                        user_input.enter_down = false;
                        break;
                    }
                    default: break;
                }
            }
            break;
            case SDL_KEYDOWN: {
                user_input.some_key_pressed = true;
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: {
                        user_input.quit = true;
                        break;
                    }
                    case SDLK_w: {
                        user_input.w_pressed = true;
                        user_input.w_down = true;
                        break;
                    }
                    case SDLK_a: {
                        user_input.a_pressed = true;
                        user_input.a_down = true;
                        break;
                    }
                    case SDLK_s: {
                        user_input.s_pressed = true;
                        user_input.s_down = true;
                        break;
                    }
                    case SDLK_d: {
                        user_input.d_pressed = true;
                        user_input.d_down = true;
                        break;
                    }
                    case SDLK_l: {
                        user_input.l_pressed = true;
                        user_input.l_down = true;
                        break;
                    }
                    case SDLK_SPACE: {
                        user_input.space_pressed = true;
                        break;
                    }
                    case SDLK_p: {
                        user_input.p_pressed = true;
                        user_input.p_down = true;
                        break;
                    }
                    case SDLK_RETURN: {
                        user_input.enter_down = true;
                        user_input.enter_pressed = true;
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
    }
}

bool detect_window_error() {
    const char* sdl_error = SDL_GetError();
    if (sdl_error && sdl_error[0] != '\0') {
        char error[256]{};
        sprintf(error, "SDL Error: %s\n", sdl_error);
        game_error_log(error);
        SDL_ClearError();
        return true;
    }	
    return false;
}