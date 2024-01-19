
#include "glad/glad.h"

/// <summary>
/// Stores a OpenGL Texture, the slot it is associated with, and its file path
/// </summary>
struct texture_t {
	GLuint id;
    int handle = -1;
    char path[1024]{};
    unsigned tex_slot = 0;
    int width = -1;
    int height = -1;
    int num_channels = -1;
};

/// <summary>
/// Create a texture
/// </summary>
/// <param name="path">File path of the texture</param>
/// <param name="tex_slot">Its texture slot</param>
/// <returns>The handle of the texture</returns>
int create_texture(const char* path, int tex_slot);

int create_texture(unsigned char* buffer, int width, int height, int tex_slot);

/// <summary>
/// Get the texture given the handle
/// </summary>
/// <param name="handle">Texture handle</param>
/// <returns>A pointer to the texture, can be NULL be doesn't exist</returns>
texture_t* get_tex(int handle);

/// <summary>
/// Bind a texture
/// </summary>
/// <param name="texture">Texture to bind</param>
void bind_texture(const texture_t& texture);

/// <summary>
/// Bind a texture
/// </summary>
/// <param name="handle">Handle of texture to bind</param>
void bind_texture(int handle, bool required_bind = false);

/// <summary>
/// Unbind the texture on what texture slot is currently active
/// </summary>
void unbind_texture();

void delete_texture(int tex_handle);
