#include "OGLTexture.h"
#include "SDL2/SDL.h"
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>
#include "imgui/imgui.h"
#include <sstream>

#ifndef _WIN32
#include <libgen.h>
#endif

static int id_num = 0;

OGLTexture::OGLTexture() : OGLImageLike(), crop_h(0), crop_w(0), handle(0), internal_format(0), data_format(GL_NONE), data_type(GL_NONE)
{

}

OGLTexture::OGLTexture(std::string filepath, int crop_w, int crop_h) : crop_w(crop_w), crop_h(crop_h), handle(0), internal_format(0), data_format(GL_NONE), data_type(GL_NONE)
{
	loadFile(filepath);
}

SDL_Surface* crop_surface(SDL_Surface* input, int x, int y, int width, int height)
{
	SDL_Surface* surface = SDL_CreateRGBSurface(input->flags, width, height, input->format->BitsPerPixel, input->format->Rmask, input->format->Gmask, input->format->Bmask, input->format->Amask);

	SDL_Rect rect = { x, y, width, height };

	SDL_BlitSurface(input, &rect, surface, 0);

	return surface;
}

void OGLTexture::loadFileRW(std::string filename, SDL_RWops* filehandle)
{
	SDL_Surface *surface = nullptr;
	SDL_Surface *cropped = nullptr;
	void *source;

	surface = IMG_Load_RW(filehandle,true);

	if (surface) {
		switch (surface->format->BytesPerPixel)
		{
		case 3:
			data_format = GL_RGB;
			break;
		case 4:
			data_format = GL_RGBA;
			break;
		default: //rut roh
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unsupported image format\n...(from %s)", filename.c_str());
			return;
		}

		internal_format = data_format;
		data_type = GL_UNSIGNED_BYTE;

		int new_width = surface->w;
		int new_height = surface->h;

		if (crop_w > 0 && crop_h > 0)
		{
			int xmargin = (new_width - crop_w) / 2;
			cropped = crop_surface(surface, xmargin, 0, crop_w, crop_h);

			new_width = crop_w;
			new_height = crop_h;
			source = cropped->pixels;
		}
		else {
			source = surface->pixels;
		}

		if (handle == 0 || new_height != height || new_width != width)
		{
			if (handle != 0) glDeleteTextures(1, &handle);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glGenTextures(1, &handle);
			glBindTexture(GL_TEXTURE_2D, handle);
			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, new_width, new_height, 0, data_format, data_type, source);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else { //update
			glBindTexture(GL_TEXTURE_2D, handle);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, new_width, new_height, data_format, data_type, source);
		}

		width = new_width;
		height = new_height;
		if (cropped != nullptr)SDL_FreeSurface(cropped);
		if (surface != nullptr)SDL_FreeSurface(surface);
	}
	else {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "IMG_Load error: %s\n...(from %s)", IMG_GetError(), filename.c_str());
	}

	name = filename;
}

void OGLTexture::loadFile(fs::path filepath)
{
	loadFileRW(filepath.filename().string(), SDL_RWFromFile(filepath.string().c_str(), "rb"));
}


OGLTexture::OGLTexture(int width, int height, std::string name, GLint internal_format, GLenum data_format) : 
	OGLImageLike(width, height), name(name), internal_format(internal_format), data_format(data_format)
{
	data_type = GL_FLOAT;

	makeEmptyTexture();
	
	if(name.size() <= 0)
	{
		std::stringstream fboname;
		fboname << "FBO " << id_num++;
		name = fboname.str();
	}
}

void OGLTexture::resize(int _width, int _height)
{
	width = _width;
	height = _height;
	glDeleteTextures(1, &handle);
	makeEmptyTexture();
}


int OGLTexture::getWidth() const
{
	return width;
}

int OGLTexture::getHeight() const
{
	return height;
}

void OGLTexture::setCrop(int _crop_w, int _crop_h)
{
	crop_w = _crop_w;
	crop_h = _crop_h;
}

void OGLTexture::debugDraw() const
{
	ImGui::Image((ImTextureID)handle, ImVec2(width, height));

	std::string popupname = name + "popup";

	if (ImGui::BeginPopupContextItem(popupname.c_str()))
	{
		ImGui::Text("%s", name.c_str());
		ImGui::Separator();
		if (ImGui::Selectable("Save")) saveTo("./shaders/debug", name);
		ImGui::Text("%d x %d", width, height);
		ImGui::EndPopup();
	}
}

GLuint OGLTexture::getHandle() const
{
	return handle;
}

SDL_Surface* OGLTexture::getAsSurface() const
{
	static const unsigned int amask = 0xFF000000;
	static const unsigned int bmask = 0x00FF0000;
	static const unsigned int gmask = 0x0000FF00;
	static const unsigned int rmask = 0x000000FF;

	SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
	SDL_LockSurface(surface);
	glBindTexture(GL_TEXTURE_2D, handle);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	SDL_UnlockSurface(surface);
	return surface;
}

void OGLTexture::saveTo(std::string folder, std::string filename) const
{
	SDL_Surface* surface = getAsSurface();
	IMG_SavePNG(surface, (folder + "/" + filename + ".png").c_str());
	SDL_FreeSurface(surface);
}

void OGLTexture::makeEmptyTexture()
{
	glGenTextures(1, &handle);

	glBindTexture(GL_TEXTURE_2D, handle);

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, data_type, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


OGLTexture::~OGLTexture()
{
}


