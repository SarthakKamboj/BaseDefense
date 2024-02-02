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

    memset(user_input.pressed, 0, sizeof(user_input.pressed));
    memset(user_input.released, 0, sizeof(user_input.released));

    user_input.quit = false;
    user_input.mouse_scroll_wheel_delta_y = 0; 
    
    user_input.controller_x_axis = 0;
    user_input.controller_y_axis = 0;

    if (user_input.game_controller) {
        user_input.controller_x_axis = SDL_GameControllerGetAxis(user_input.game_controller, SDL_CONTROLLER_AXIS_LEFTX) / 32768.f;
        user_input.controller_y_axis = SDL_GameControllerGetAxis(user_input.game_controller, SDL_CONTROLLER_AXIS_LEFTY) / -32768.f;

        if (abs(user_input.controller_x_axis) < 0.1) {
            user_input.controller_x_axis = 0;
        }
        if (abs(user_input.controller_y_axis) < 0.1) {
            user_input.controller_y_axis = 0;
        }
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
                        window.window_width = event.window.data1;
                        window.window_height = event.window.data2;
                        window.resized = true;
                        glViewport(0, 0, window.window_width, window.window_height);
                        printf("new window size: (%f, %f)", window.window_width, window.window_height);
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
                        for (KEYS controller_key = CONTROLLER_A; controller_key < NUM_KEYS; controller_key++) {
                            user_input.down[controller_key] = false;
                            user_input.released[controller_key] = false;
                        }
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
                                user_input.released[CONTROLLER_A] = true;
                                user_input.down[CONTROLLER_A] = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: {
                                user_input.released[CONTROLLER_Y] = true;
                                user_input.down[CONTROLLER_Y] = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: {
                                user_input.released[CONTROLLER_B] = true;
                                user_input.down[CONTROLLER_B] = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: {
                                user_input.released[CONTROLLER_X] = true;
                                user_input.down[CONTROLLER_X] = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: {
                                user_input.released[CONTROLLER_START] = true;
                                user_input.down[CONTROLLER_START] = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: {
                                user_input.released[CONTROLLER_LB] = true;
                                user_input.down[CONTROLLER_LB] = false;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: {
                                user_input.released[CONTROLLER_RB] = true;
                                user_input.down[CONTROLLER_RB] = false;
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
                    SDL_Joystick* controller_joystick = SDL_GameControllerGetJoystick(user_input.game_controller);
                    if (event.cdevice.which == SDL_JoystickInstanceID(controller_joystick)) {
                        switch (event.cbutton.button) {
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: {
                                user_input.down[CONTROLLER_A] = true;
                                user_input.pressed[CONTROLLER_A] = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: {
                                user_input.down[CONTROLLER_Y] = true;
                                user_input.pressed[CONTROLLER_Y] = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: {
                                user_input.down[CONTROLLER_B] = true;
                                user_input.pressed[CONTROLLER_B] = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: {
                                user_input.down[CONTROLLER_X] = true;
                                user_input.pressed[CONTROLLER_X] = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: {
                                user_input.down[CONTROLLER_START] = true;
                                user_input.pressed[CONTROLLER_START] = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: {
                                user_input.down[CONTROLLER_LB] = true;
                                user_input.pressed[CONTROLLER_LB] = true;
                                break;
                            }
                            case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: {
                                user_input.down[CONTROLLER_RB] = true;
                                user_input.pressed[CONTROLLER_RB] = true;
                                break;
                            }
                            default: break;
                        }
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    user_input.released[RIGHT_MOUSE] = true;
                    user_input.down[RIGHT_MOUSE] = false;
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    user_input.released[LEFT_MOUSE] = true;
                    user_input.down[LEFT_MOUSE] = false;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    user_input.released[MIDDLE_MOUSE] = true;
                    user_input.down[MIDDLE_MOUSE] = false;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    user_input.pressed[RIGHT_MOUSE] = true;
                    user_input.down[RIGHT_MOUSE] = true;
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    user_input.pressed[LEFT_MOUSE] = true;
                    user_input.down[LEFT_MOUSE] = true;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    user_input.pressed[MIDDLE_MOUSE] = true;
                    user_input.down[MIDDLE_MOUSE] = true;
                }
                break;
            }
            case SDL_KEYUP: {
                if (event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z') {
                    user_input.released[event.key.keysym.sym - 'a'] = true;
                    user_input.down[event.key.keysym.sym - 'a'] = false;
                }
            }
            break;
            case SDL_KEYDOWN: {
                if (event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z') {
                    user_input.down[event.key.keysym.sym - 'a'] = true;
                    user_input.pressed[event.key.keysym.sym - 'a'] = true;
                }
            }
            break;
            case SDL_MOUSEWHEEL: {
                user_input.mouse_scroll_wheel_delta_y = event.wheel.y;
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

bool get_pressed(KEYS key) {
    return globals.window.user_input.pressed[key];
}

bool get_down(KEYS key) {
    return globals.window.user_input.down[key];
}

bool get_released(KEYS key) {
    return globals.window.user_input.released[key];
}

KEYS operator++(KEYS& a, int) {
    return static_cast<KEYS>(static_cast<int>(a) + 1);
}

bool is_controller_connected() {
    return globals.window.user_input.game_controller != NULL;
}