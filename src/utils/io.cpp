#include "io.h"

#include <fstream>
#include <iostream>
#include <windows.h>

#include "constants.h"

static bool running_in_vs = false;

std::string get_file_contents(const char* path) {
	std::ifstream file(path, std::ios::binary);
	file.seekg(0, file.end);
	int size = file.tellg();
	file.seekg(0, file.beg);
	std::ios_base::iostate state = file.rdstate();
	if (state != std::ios_base::goodbit) {
		char error[512]{};
		sprintf(error, "file with path %s not opening properly", path);
		game_assert_msg(false, error);
	}
	while (!file.eof()) {
		char* data = new char[(int)size + 1];
		memset(data, 0, size + 1);
		file.read(data, size);
		std::string fileStr(data);
		delete[] data;
		file.close();
		return fileStr;
	}
	return std::string();
}

void set_running_in_visual_studio(bool _running_in_vs) {
	running_in_vs = _running_in_vs;
}

void get_resources_folder_path(char path_buffer[256]) {
	if (running_in_vs) {
		sprintf(path_buffer, "C:\\Sarthak\\projects\\base_defense\\resources");
		return;
	}
	static bool got_root_path = false;
	static char s_folder[256]{};
	if (!got_root_path) {
		char folder[256]{};
		GetCurrentDirectoryA(256, folder);
		sprintf(s_folder, "%s\\resources", folder);
		got_root_path = true;
	}
	memcpy(path_buffer, s_folder, strlen(s_folder));
}