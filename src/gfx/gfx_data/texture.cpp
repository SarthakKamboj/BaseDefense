#include "texture.h"

#include "stb/stb_image.h"

#include "constants.h"
#include "gfx/renderer.h"

#include <vector>

static std::vector<texture_t> textures;
static int tex_running_cnt = 0;

// TEXTURE
int create_texture(const char* path, int tex_slot) {
    for (int i = 0; i < textures.size(); i++) {
        if (strcmp(path, textures[i].path) == 0) {
            return textures[i].handle;
        }
    }

	texture_t texture{};
    texture.tex_slot = tex_slot;
	// int num_channels, width, height;
	stbi_set_flip_vertically_on_load(true);

	unsigned char* data = stbi_load(path, &texture.width, &texture.height, &texture.num_channels, 0);
	game_assert(data);

	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (texture.num_channels == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);
    texture.handle = tex_running_cnt;
    tex_running_cnt++;
	memcpy(texture.path, path, strlen(path));

    textures.push_back(texture);

	game_assert(!detect_gfx_error());

	return texture.handle;
}

int create_texture(unsigned char* buffer, int width, int height, int tex_slot) {
	texture_t texture;
	texture.handle = tex_running_cnt;
	tex_running_cnt++;

	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, (void*)buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	texture.tex_slot = tex_slot;

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		// Log or print the OpenGL error
		std::cout << "ran into opengl error" << std::endl;
	}

	textures.push_back(texture);

	return texture.handle;
}

void bind_texture(const texture_t& texture) {
    glActiveTexture(GL_TEXTURE0 + texture.tex_slot);
	glBindTexture(GL_TEXTURE_2D, texture.id);
}

void bind_texture(int handle, bool required_bind) {
    for (int i = 0; i < textures.size(); i++) {
        if (textures[i].handle == handle) {
            glActiveTexture(GL_TEXTURE0 + textures[i].tex_slot);
	        glBindTexture(GL_TEXTURE_2D, textures[i].id);
            return;
        }
    }

	if (required_bind) {
		// game_assert("could not find texture even though required bind was asserted");
		game_assert(false);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void unbind_texture() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

texture_t* get_tex(int handle) {
    for (int i = 0; i < textures.size(); i++) {
        if (textures[i].handle == handle) {
            return &textures[i];
        }
    }
    return NULL;
}