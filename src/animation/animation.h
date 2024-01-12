#pragma once

#include <vector>

#include "utils/time.h"


struct animation_t {
    int handle = -1;
    // start and end values of the float the animation is animating
	float start = 0.f, end = 0.f;
    // pointer to the value the animation is animating
	float* cur = nullptr;
	float runtime = 0.f;
    time_count_t time_elapsed = 0.f;
    time_count_t start_time = 0.f;
    // whether animation is active or not
	bool enabled = false;
};

int create_animation(float* value, float start_val, float end_val, float runtime);
animation_t* get_animation(int anim_handle);
void update_animations();
void start_animation(int anim_handle);

struct image_anim_t {
    int handle = -1;
    std::vector<int> textures;    
    char name[64]{};
};

int create_image_anim(const char* anim_folder, const char* name);
image_anim_t* get_image_anim(int anim_handle);
image_anim_t* get_image_anim(const char* anim_name);
// int get_tex_handle_for_image_anim(int anim_handle);

struct image_anim_player_t {
    int handle = -1;
    int cur_frame = 0;
    int image_anim_handle = -1;
};
int create_image_anim_player(int image_anim_handle);
image_anim_player_t* get_anim_player(int image_anim_player_handle);
void update_image_anim_players();

struct anim_state_machine_t {
    int handle = -1;
    std::vector<int> image_animation_players;
    int cur_anim_playing = -1;
    char name[256]{};
};

int create_state_machine(char* state_machine_folder, const char* name, const char* starting_anim_name);
int get_tex_handle_for_statemachine(int state_machine_handle);
void set_state_machine_anim(int state_machine_handle, const char* anim_name);
void update_state_machines();