#pragma once
#include "OGLRenderStage.h"
#include "OGLTexture.h"
#include "OGLShader.h"

class RenderStageMaxPool :
	public OGLRenderStage
{
public:
	RenderStageMaxPool(std::shared_ptr<OGLTexture> source);
	virtual ~RenderStageMaxPool();
	void draw(OGLRenderStage *previous_fbo);
	void debugDisplay();

private:
	GLuint vao;
	GLuint u_input_size;
	GLuint input_size;
	OGLShader pool_shader;
};


