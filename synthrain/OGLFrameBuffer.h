#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <memory>
#include <vector>
#include "OGLTexture.h"
#include "OGLFloatTexture.h"

class OGLFrameBuffer : public OGLImageLike
{
public:
	OGLFrameBuffer(int width, int height, std::string name = "");
	OGLFrameBuffer();
	void initialize(int width, int height, std::string name = "");
	void initializeWithFloat(int width, int height, std::string name = "");
	void resize(int width, int height);
	void saveTo(std::string folder, std::string filename) const;
	void activate();
	void deactivate();

	void addDepthBuffer(std::shared_ptr<OGLFloatTexture> tex);
	void addColorBuffer();

	void debugDraw() const;
	GLuint getHandle(unsigned int idx = 0) const;

	SDL_Surface* getAsSurface() const override;
	SDL_Surface* getAsSurface(unsigned int idx = 0) const;
	std::shared_ptr<OGLTexture> getColorBuffer(unsigned int idx = 0);


	~OGLFrameBuffer();
private:

	void regenerateBuffer();
	GLuint framebuffer;
	std::vector<std::shared_ptr<OGLTexture>> color_buffers;
	std::shared_ptr<OGLFloatTexture> depth_buffer;
	std::string name;

	bool has_depth;

};

