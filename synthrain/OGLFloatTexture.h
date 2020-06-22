#pragma once

#include "OGLTexture.h"
#include "fvector2.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <SDL2/SDL.h>
#include <array>
#include <vector>

class OGLFloatTexture : public OGLTexture
{
public:

	OGLFloatTexture(GLenum format, fs::path filepath = "");
	OGLFloatTexture(GLenum format, int width, int height, std::string name = "");

	virtual void loadFileRW(std::string filename, SDL_RWops* filehandle);
	
	std::array<fvector2, 9> getFlowData();
	operator GLuint() { return handle; }

	std::vector<float> valueAt(int x, int y);

protected:
	OGLFloatTexture();
	int num_channels;

	std::array<fvector2, 9> flow;

	std::vector<char> raw_data;
};

