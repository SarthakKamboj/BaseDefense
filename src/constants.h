#pragma once

#include <iostream>
#include <cassert>

#define STARTING_WINDOW_WIDTH 800
#define STARTING_WINDOW_HEIGHT 600
#define GRAVITY (9.8f * 100.f)

#define LEVEL_MAP_GRID_SIZE 16
#define GAME_GRID_SIZE 40

#define AUDIO_FOLDER "audio"
#define LEVELS_FOLDER "levels"
#define ART_FOLDER "art"
#define FONTS_FOLDER "fonts"
#define SHADERS_FOLDER "shaders"
#define UI_FOLDER "ui"

#define BACKGROUND_MUSIC "eden_electro.wav"
#define JUMP_SOUND_EFFECT "jump.wav"
#define STOMP_SOUND_EFFECT "stomp.wav"
#define LEVEL_FINISH_SOUND_EFFECT "level_finish.wav"

#define UI_TESTING

#define MAX(a,b) ((a) > (b)) ? a : b

#define TEST_UI_LEVEL -1
#define LEVEL_1 1
#define LEVEL_2 2 
#define LEVEL_3 3 
#define LEVEL_4 4
#define LEVEL_5 5
#define MAIN_MENU_LEVEL 1022
#define LEVELS_DISPLAY 1023
#define GAME_OVER_SCREEN_LEVEL 1024
#define SETTINGS_LEVEL 1025
#define QUIT_LEVEL 1026
#define CREDITS_LEVEL 1027
#define HIGH_SCORES_LEVEL 1028

// #define game_assert(exp) if ((exp) == false) std::cout << "on line " << __LINE__ << " in file " << __FILE__ << " the game_assert failed" << std::endl;
#define game_assert(exp) assert(exp)
#define game_assert_msg(exp, msg) assert(exp && msg)
#define game_throw_error() assert(false)

#define game_info_log(msg) printf(msg)
#define game_error_log(msg) printf("ERROR: %s", msg)

#define OR_ENUM_DECLARATION(ENUM_TYPE) \
    inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) { \
        return static_cast<ENUM_TYPE>(static_cast<int>(a) | static_cast<int>(b)); \
    }

#define _TESTING 0