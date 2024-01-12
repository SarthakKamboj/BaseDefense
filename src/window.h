#pragma once

#include "SDL.h"

enum ASPECT_RATIO {
    A_1600x900 = 0,
    A_1600x1000,
    A_1600x1200,

    NUM_RATIOS,
};

struct aspect_ratio_t {
    ASPECT_RATIO ratio;
    const char* str;
    float width = 0.f;
    float height = 0.f;
    int mode_index = -1;
};

int get_mode_index(float w, float h);

/// <summary>
/// Struct to store user input data
/// </summary>
struct user_input_t {

    bool some_key_pressed = false;

    // true just on the initial click, but false even if button is down
    bool w_pressed = false;
    bool a_pressed = false;
    bool s_pressed = false;
    bool d_pressed = false;
    bool p_pressed = false;
    bool l_pressed = false;
    bool space_pressed = false;
    bool enter_pressed = false;
    
    bool controller_a_pressed = false;
    bool controller_y_pressed = false;
    bool controller_x_pressed = false;
    bool controller_b_pressed = false;
    bool controller_start_pressed = false;

    // true while continuously button is down
    bool w_down = false;
    bool a_down = false;
    bool s_down = false;
    bool d_down = false;
    bool p_down = false;
    bool l_down = false;
    bool enter_down = false;

    bool controller_a_down = false;
    bool controller_y_down = false;
    bool controller_x_down = false;
    bool controller_b_down = false;
    bool controller_start_down = false;

    bool right_clicked = false;
    bool left_clicked = false;

    bool quit = false;

    int x_pos;
    int y_pos;

    float controller_x_axis = 0;
    float controller_y_axis = 0;

	bool controller_state_changed = false;
	SDL_GameController* game_controller = NULL;
};


struct window_t {
    float window_width = 0;
	float window_height = 0;
	bool resized = false;
	bool is_full_screen = true;

	SDL_Window* sdl_window = NULL;

    user_input_t user_input;
};

void init_window();
void clear_window();
void render_window();

/// <summary>
/// process the input for the frame
/// </summary>
void process_input();

bool detect_window_error();