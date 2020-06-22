#pragma once
#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <SDL2/SDL.h>

#include <filesystem>
#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif


class OGLImageLike
{
public:
	OGLImageLike() : width(0), height(0) {}
	OGLImageLike(int width, int height) : width(width), height(height) {}
	virtual void resize(int width, int height) = 0;
	virtual void saveTo(std::string folder, std::string filename) const = 0;
	virtual void debugDraw() const = 0;
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	virtual bool valid() { return width != 0 && height != 0; }

	bool sameSize(const OGLImageLike & other, unsigned int divisor) const
	{
		return width / divisor == other.width && height / divisor == other.height;
	}

	virtual SDL_Surface* getAsSurface() const = 0;

protected:
	int width;
	int height;
};


class OGLTexture : public OGLImageLike
{
public:
	OGLTexture();
	OGLTexture(std::string filepath, int crop_w = 0, int crop_h = 0);
	OGLTexture(int width, int height, std::string name = "", GLint format = GL_RGBA32F, GLenum layout = GL_RGBA);

	virtual void loadFileRW(std::string filename, SDL_RWops* filehandle);
	void loadFile(fs::path filepath);
	
	virtual void resize(int width, int height);

	operator GLuint() { return handle; }
	
	int getWidth() const;
	int getHeight() const;

	void setCrop(int crop_w, int crop_h);

	~OGLTexture();
	void debugDraw() const;
	GLuint getHandle() const;

	void saveTo(std::string folder, std::string filename) const;

	SDL_Surface* getAsSurface() const override;

protected:
	void makeEmptyTexture();

	int crop_h;
	int crop_w;

	GLuint handle;
	GLint internal_format;
	GLenum data_format;
	GLenum data_type;
	std::string name;
};