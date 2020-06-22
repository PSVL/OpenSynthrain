#pragma once
#include "OGLFrameBuffer.h"
#include "OGLTexture.h"
#include <string>

class OGLRenderStage
{
public:
	OGLRenderStage(std::shared_ptr<OGLTexture> source, unsigned int divisor, std::string name);
	virtual ~OGLRenderStage();
	virtual void checkFBOSize();
	virtual void draw(OGLRenderStage *previous) = 0;
	virtual void debugDisplay();
	OGLFrameBuffer* getFBO();
	std::string getName();

	void setSource(std::shared_ptr<OGLTexture> source);

	static void InitSharedResources();

protected:
	OGLFrameBuffer fbo;
	std::shared_ptr<OGLTexture> source;
	unsigned int source_divisor;
	std::string name;

	static GLuint vao_fs_quad;
	static GLuint vao_center_origin_quad;
};