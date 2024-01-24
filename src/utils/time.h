#pragma once

#include <chrono>

typedef double time_count_t;
typedef std::chrono::duration<time_count_t, std::ratio<1>> game_time_t;

/// <summary>
/// A timer to track how much time has elapsed and whether it is running or not
/// </summary>
struct game_timer_t {
    game_time_t start_time;
    game_time_t end_time;
    time_count_t elapsed_time_sec;
    bool running = false;
    char msg[256]{};

    ~game_timer_t();
};

void create_debug_timer(const char* msg, game_timer_t& debug_timer);

/// <summary>
/// Start the timer
/// </summary>
/// <param name="timer">Timer to start</param>
void start_timer(game_timer_t& timer);

/// <summary>
/// End a timer and record its time elapsed
/// </summary>
/// <param name="timer">Timer to end</param>
void end_timer(game_timer_t& timer);

/// <summary>
/// Application wide singletons
/// </summary>
namespace game {
	/// <summary>
	/// Keep track of current time in game and delta time of the frame
	/// </summary>
	struct time_t {
		static time_count_t delta_time;
		static time_count_t cur_time;
		static time_count_t game_cur_time;
	};
}
