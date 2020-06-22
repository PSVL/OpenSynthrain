#include "OGLFrameBuffer.h"
#include "OGLFloatTexture.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <cassert>
#include <string>
#include <sstream>

std::string makeBufferName(std::string name, int count)
{
	std::ostringstream out;
	out << name.c_str() << ":" << ++count;
	return out.str();
}

OGLFrameBuffer::OGLFrameBuffer(int width, int height, std::string name) : OGLImageLike(width,height), name(name), framebuffer(0), depth_buffer(nullptr)
{
	color_buffers.push_back(std::make_shared<OGLTexture>(width, height, makeBufferName(name,color_buffers.size())));
	regenerateBuffer();
}

OGLFrameBuffer::OGLFrameBuffer()
{

}

void OGLFrameBuffer::initialize(int _width, int _height, std::string _name)
{
	SDL_assert_always(color_buffers.empty() == true);

	name = _name;
	width = _width;
	height = _height;
	color_buffers.push_back(std::make_shared<OGLTexture>(width, height, makeBufferName(name, color_buffers.size())));
	regenerateBuffer();
}

void OGLFrameBuffer::initializeWithFloat(int _width, int _height, std::string _name)
{
	SDL_assert_always(color_buffers.empty() == true);

	name = _name;
	width = _width;
	height = _height;
	color_buffers.push_back(std::make_shared<OGLFloatTexture>(GL_RED, width, height, makeBufferName(name, color_buffers.size())));
	regenerateBuffer();
}

void OGLFrameBuffer::addDepthBuffer(std::shared_ptr<OGLFloatTexture> tex)
{
	depth_buffer = tex;
	regenerateBuffer();
}

void OGLFrameBuffer::addColorBuffer()
{
	color_buffers.push_back(std::make_shared<OGLTexture>(width, height, makeBufferName(name, color_buffers.size())));
	regenerateBuffer();
}

void OGLFrameBuffer::resize(int _width, int _height)
{
	width = _width;
	height = _height;

	if(framebuffer != 0)glDeleteFramebuffers(1, &framebuffer);

	for (auto buffer : color_buffers)
	{
		buffer->resize(_width, _height);
	}

	regenerateBuffer();
}

void OGLFrameBuffer::saveTo(std::string folder, std::string filename) const
{
	unsigned int idx = 0;
	for (auto buffer : color_buffers)
	{
		std::ostringstream name;
		name << filename << "_" << idx++;
		buffer->saveTo(folder, filename.c_str());
	}
}

void OGLFrameBuffer::activate()
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, width, height);
}

void OGLFrameBuffer::deactivate()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OGLFrameBuffer::debugDraw() const
{
	for (auto buffer : color_buffers)
	{
		buffer->debugDraw();
	}
}

GLuint OGLFrameBuffer::getHandle(unsigned int idx) const
{
	SDL_assert_always(idx < color_buffers.size());

	return color_buffers[idx]->getHandle();
}

SDL_Surface * OGLFrameBuffer::getAsSurface() const
{
	return getAsSurface(0);
}

SDL_Surface * OGLFrameBuffer::getAsSurface(unsigned int idx) const
{
	SDL_assert_always(idx < color_buffers.size());

	return color_buffers[idx]->getAsSurface();
}

std::shared_ptr<OGLTexture> OGLFrameBuffer::getColorBuffer(unsigned int idx)
{
	SDL_assert_always(idx < color_buffers.size());
	if (color_buffers.size() > idx) return color_buffers[idx];
	return std::shared_ptr<OGLTexture>(nullptr);
}

OGLFrameBuffer::~OGLFrameBuffer()
{

}

void OGLFrameBuffer::regenerateBuffer()
{
	if (framebuffer != 0)glDeleteFramebuffers(1, &framebuffer);

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	std::vector<GLenum> bufferlist;

	{
		unsigned int idx = 0;
		for (auto buffer : color_buffers)
		{
			SDL_assert_release(buffer->valid() == true);
			GLenum attach = GL_COLOR_ATTACHMENT0 + idx++;
			glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, buffer->getHandle(), 0);
			auto err = glGetError();
			assert(err == GL_NO_ERROR);
			bufferlist.push_back(attach);
		}
	}

	if (depth_buffer != nullptr)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_buffer->getHandle(), 0);
	}

	glDrawBuffers(bufferlist.size(), &bufferlist[0]);

	
	if (auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE)
	{
		SDL_Log("Framebuffer incomplete (%s)", name.c_str());
	}	
}
