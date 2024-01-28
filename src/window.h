#pragma once

#include "SDL.h"

enum ASPECT_RATIO {
    A_1600x900 = 0,
    A_1600x1000,
    A_1600x1200,

    NUM_RATIOS,
};

enum KEYS {
    KEY_A = 0,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    KEY_SPACE,
    KEY_ENTER,

    LEFT_MOUSE,
    MIDDLE_MOUSE,
    RIGHT_MOUSE,

    CONTROLLER_A,
    CONTROLLER_Y,
    CONTROLLER_B,
    CONTROLLER_X,
    CONTROLLER_START,
    CONTROLLER_LB,
    CONTROLLER_RB,

    NUM_KEYS
};
KEYS operator++(KEYS& a, int);

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

    // first time key down
    bool pressed[NUM_KEYS]{};
    // true while key is down
    bool down[NUM_KEYS]{};
    // true when key is released
    bool released[NUM_KEYS]{};
    
    // true while continuously button is down
    bool enter_down = false;

    // bool right_mouse_down_click = false;
    // bool left_mouse_down_click = false;
    // bool middle_mouse_down_click = false;
    // bool right_mouse_release = false;
    // bool left_mouse_release = false;
    // bool middle_mouse_release = false;
    // bool right_mouse_down = false;
    // bool left_mouse_down = false;
    // bool middle_mouse_down = false;

    bool quit = false;

    // positive away from user
    float mouse_scroll_wheel_delta_y = 0;

    // bottom left is (0,0)
    int mouse_x;
    int mouse_y; 

	bool controller_state_changed = false;
	SDL_GameController* game_controller = NULL;
    float controller_x_axis = 0;
    float controller_y_axis = 0;
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
bool get_pressed(KEYS key);
bool get_down(KEYS key);
bool get_released(KEYS key);

bool detect_window_error();
bool is_controller_connected();