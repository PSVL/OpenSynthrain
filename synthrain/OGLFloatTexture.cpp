#include "OGLFloatTexture.h"
#include "SDL2/SDL.h"
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>
#include "imgui/imgui.h"
#include <sstream>
#include <fstream>
#include <vector>

#ifndef _WIN32
#include <libgen.h>
#endif

static int id_num = 0;

int format_to_channels(GLenum format)
{
	int out = 0;

	switch (format)
	{
		case GL_RED:
		case GL_DEPTH_COMPONENT:
			out = 1;
			break;
		case GL_RG:
			out = 2;
			break;
		case GL_RGB:
			out = 3;
			break;
		case GL_RGBA:
			out = 4;
			break;
	}

	SDL_assert_always(out != 0);
	return out;
}

GLint format_to_internal(GLenum format)
{
	GLint out = GL_NONE;
	switch (format)
	{
	case GL_DEPTH_COMPONENT:
		out = GL_DEPTH_COMPONENT32F;
		break;
	case GL_RED:
		out = GL_R32F;
		break;
	case GL_RG:
		out = GL_RG32F;
		break;
	case GL_RGB:
		out = GL_RGB32F;
		break;
	case GL_RGBA:
		out = GL_RGBA32F;
		break;
	}

	SDL_assert_always(out != GL_NONE);
	return out;
}

OGLFloatTexture::OGLFloatTexture() : OGLTexture(), flow{ 0 }  
{

}

OGLFloatTexture::OGLFloatTexture(GLenum format, fs::path filepath) : OGLTexture(), flow{0}, num_channels(format_to_channels(format))
{
	data_format = format;
	internal_format = format_to_internal(data_format);

	if(filepath.empty() == false) loadFile(filepath);
}

void OGLFloatTexture::loadFileRW(std::string filename, SDL_RWops *io)
{
	data_type = GL_FLOAT;

	if (io != NULL) {
		size_t size = SDL_RWsize(io);
		if (size > 0)
		{
			raw_data.resize(size);
			if (SDL_RWread(io, &raw_data[0], raw_data.size(), 1) > 0) {
				//SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "Loaded float file: %s", path);
			}
			SDL_RWclose(io);
		}
		else {
			SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "Flow file was empty or invalid: %s", filename.c_str());
			return;
		}
	}
	else {
		SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "Failed to open flow file: %s", filename.c_str());
		return;
	}

	if (raw_data.size() > 0) {
		int new_width = *((float*)&raw_data[0]);
		int new_height = *((float*)&raw_data[4]);;
	
		if (handle == 0 || new_height != height || new_width != width)
		{
			if (handle != 0) glDeleteTextures(1, &handle);

			width = new_width;
			height = new_height;

			glGenTextures(1, &handle);

			glBindTexture(GL_TEXTURE_2D, handle);
		//	glTexStorage2D(GL_TEXTURE_2D, 10, color_format, width, height);
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, color_mode, data_type, &out[8]);

			GLenum err;


			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, data_type,	&raw_data[8]);

			//glGenerateMipmap(GL_TEXTURE_2D); //free incremental downsample

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			//glGetTexImage(GL_TEXTURE_2D, 8, color_mode, data_type, flow.data());
		} else {
			width = new_width;
			height = new_height;

			glBindTexture(GL_TEXTURE_2D, handle);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, data_format, data_type, &raw_data[8]);
			glGenerateMipmap(GL_TEXTURE_2D); //free inremental downsample
		}
	}
	else {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Flow iamge error: %s\n...(from %s)", IMG_GetError(), filename.c_str());
	}

	name = filename;
}

std::array<fvector2, 9> OGLFloatTexture::getFlowData()
{
	return flow;
}

std::vector<float> OGLFloatTexture::valueAt(int x, int y)
{
	size_t idx = y*width+x;
	std::vector<float> out;
	float* pixel = &((float*)&raw_data[8])[idx*num_channels];
	for (int i = 0; i < num_channels; ++i) out.push_back(pixel[i]);
	return out;
}

OGLFloatTexture::OGLFloatTexture(GLenum format, int width, int height, std::string name) : OGLTexture(width, height,name, format_to_internal(format), format), num_channels(format_to_channels(format))
{
	makeEmptyTexture();
	
	if(name.size() <= 0)
	{
		std::stringstream fboname;
		fboname << "FBO " << id_num++;
		name = fboname.str();
	}
}


