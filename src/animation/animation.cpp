#include "animation.h"
#include "gfx/gfx_data/texture.h"
#include "gfx/gfx_data/shader.h"
#include "utils/time.h"
#include <vector>
#include <iostream>
#include <filesystem>
#include <string.h>

#include "constants.h"

namespace fs = std::filesystem;

std::vector<animation_t> animations;
std::vector<image_anim_t> image_anims;
std::vector<image_anim_player_t> image_anim_players;
std::vector<anim_state_machine_t> state_machines;

void update_animations() {
	for (animation_t& animation : animations) {
		if (animation.enabled) {
			auto cur = std::chrono::high_resolution_clock::now();
			animation.time_elapsed = std::chrono::duration_cast<game_time_t>(cur.time_since_epoch()).count() - animation.start_time;
			animation.enabled = animation.time_elapsed <= animation.runtime;
			if (animation.enabled) {
				*animation.cur = (animation.time_elapsed / animation.runtime) * (animation.end - animation.start) + animation.start;
			}
		}
	}
}

animation_t* get_animation(int anim_handle) {
    for (int i = 0; i < animations.size(); i++) {
        if (anim_handle == animations[i].handle) {
            return &animations[i];
        }
    }
    return NULL;
}

void start_animation(int anim_handle) {
    animation_t* anim_ptr = get_animation(anim_handle);
    assert(anim_ptr != NULL);
	animation_t& animation = *anim_ptr;
	animation.time_elapsed = 0.f;
	animation.enabled = true;
	// animation.start_time = platformer::get_time_since_start_in_sec();
	auto cur = std::chrono::high_resolution_clock::now();
	animation.start_time = std::chrono::duration_cast<game_time_t>(cur.time_since_epoch()).count();
}

int create_animation(float* value, float start_val, float end_val, float runtime) {
    static int running_cnt = 0;
    assert(value != NULL);
	animation_t animation;
    animation.handle = running_cnt;
	animation.cur = value;
	animation.start = start_val;
	animation.end = end_val;
	animation.enabled = false;
	animation.runtime = runtime;
    running_cnt++;
	animations.push_back(animation);
	return animation.handle;
}

int create_image_anim(const char* anim_folder, const char* anim_name) {
	static int running_cnt = 0;
	image_anim_t image_anim;
	memcpy(image_anim.name, anim_name, strlen(anim_name));
	image_anim.handle = running_cnt;
	running_cnt++;
	for (auto& file : fs::directory_iterator(anim_folder)) {
		image_anim.textures.push_back(create_texture(file.path().string().c_str(), 0));
	}
	image_anims.push_back(image_anim);
	return image_anim.handle;
}

// int get_tex_handle_for_image_anim(int anim_handle) {
// 	for (image_anim_t& image_anim : image_anims) {
// 		if (image_anim.handle == anim_handle) {
// 			return image_anim.textures[image_anim.cur_frame];
// 		}
// 	}
// 	assert("image anim not found");
// }

image_anim_t* get_image_anim(int anim_handle) {
	for (image_anim_t& image_anim : image_anims) {
		if (image_anim.handle == anim_handle) {
			return &image_anim;
		}
	}
	return NULL;
}

image_anim_t* get_image_anim(const char* anim_name) {
	for (image_anim_t& image_anim : image_anims) {
		if (strcmp(image_anim.name, anim_name) == 0) {
			return &image_anim;
		}
	}
	return NULL;
}

int create_image_anim_player(int image_anim_handle) {
	image_anim_player_t image_anim_player;
	static int running_cnt = 0;
	image_anim_player.handle = running_cnt;
	running_cnt++;
	image_anim_player.cur_frame = 0;
	image_anim_player.image_anim_handle = image_anim_handle;
	image_anim_players.push_back(image_anim_player);
	return image_anim_player.handle;
}

void update_image_anim_players() {
	auto cur = std::chrono::high_resolution_clock::now();
	static bool started = false;
	static time_count_t prev_time;
	if (!started) {
		prev_time = std::chrono::duration_cast<game_time_t>(cur.time_since_epoch()).count();
		started = true;
	}
	time_count_t cur_time = std::chrono::duration_cast<game_time_t>(cur.time_since_epoch()).count();
	if (cur_time - prev_time >= (1/10.f)) {
		for (image_anim_player_t& image_anim_player : image_anim_players) {
			image_anim_t* anim = get_image_anim(image_anim_player.image_anim_handle);
			image_anim_player.cur_frame = (image_anim_player.cur_frame + 1) % anim->textures.size();
		}
		prev_time = cur_time;
	}
}

image_anim_player_t* get_anim_player(int image_anim_player_handle) {
	for (image_anim_player_t& player : image_anim_players) {
		if (player.handle == image_anim_player_handle) return &player;
	}
	return NULL;
}

int create_state_machine(char* state_machine_folder, const char* name, const char* starting_anim_name) {
	static int running_cnt = 0;
	anim_state_machine_t state_machine;
	state_machine.handle = running_cnt;
	running_cnt++;

	char* go_name = strrchr(state_machine_folder, static_cast<int>('\\')) + 1;
	for (auto& image_anim_folder : fs::directory_iterator(state_machine_folder)) {
		auto c_folder_path = image_anim_folder.path();
		std::string c_folder_name = c_folder_path.string();
		char* folder = (char*)(c_folder_name.c_str());
		char* name = strrchr(folder, static_cast<int>('\\')) + 1;
		char full_anim_name[256]{};
		sprintf(full_anim_name, "%s_%s", go_name, name);
		image_anim_t* anim = get_image_anim(full_anim_name);
		int image_anim_handle = -1;
		if (anim) {
			image_anim_handle = anim->handle;	
		} else {
		 	image_anim_handle = create_image_anim(folder, full_anim_name);
		}
		int image_anim_player_handle = create_image_anim_player(image_anim_handle);
		if (strcmp(full_anim_name, starting_anim_name) == 0) {
			state_machine.cur_anim_playing = state_machine.image_animation_players.size();
		}
		state_machine.image_animation_players.push_back(image_anim_player_handle);
	}
	memcpy(state_machine.name, name, strlen(name));

	assert(state_machine.cur_anim_playing != -1);
	assert(state_machine.image_animation_players.size() > 0);

	state_machines.push_back(state_machine);
	return state_machine.handle;
}

void set_state_machine_anim(int state_machine_handle, const char* anim_name) {
	for (anim_state_machine_t& sm : state_machines) {
		if (sm.handle == state_machine_handle) {
			for (int i = 0; i < sm.image_animation_players.size(); i++) {
				image_anim_player_t* anim_player = get_anim_player(sm.image_animation_players[i]);
				image_anim_t* anim = get_image_anim(anim_player->image_anim_handle);
				if (strcmp(anim->name, anim_name) == 0) {
					sm.cur_anim_playing = i;
					return;
				}
			}

			printf("anim name %s for state machine %s not found", anim_name, sm.name);
			assert(false);
		}
	}	

	printf("state machine with id %i not found", state_machine_handle);
	assert(false);

}

int get_tex_handle_for_statemachine(int state_machine_handle) {
	for (anim_state_machine_t& sm : state_machines) {
		if (sm.handle == state_machine_handle) {
			image_anim_player_t* anim_player = get_anim_player(sm.image_animation_players[sm.cur_anim_playing]);
			image_anim_t* anim = get_image_anim(anim_player->image_anim_handle);
			return anim->textures[anim_player->cur_frame];
		}
	}
	return -1;
}