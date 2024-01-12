#pragma once

#include <string>
#include <cstring>

/// <summary>
/// Get the file contents in a string
/// </summary>
/// <param name="path">Absolute path of the file</param>
/// <returns>The contents of the file as a string</returns>
std::string get_file_contents(const char* path);

void set_running_in_visual_studio(bool running_in_vs);
void get_resources_folder_path(char out_buffer[256]);