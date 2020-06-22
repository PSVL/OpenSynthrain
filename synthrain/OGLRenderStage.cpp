#include "OGLRenderStage.h"
#include "SDL2/SDL.h"

GLuint OGLRenderStage::vao_fs_quad = 0;
GLuint OGLRenderStage::vao_center_origin_quad = 0;

OGLRenderStage::OGLRenderStage(std::shared_ptr<OGLTexture> source, unsigned int divisor, std::string name) : source(source), fbo(source->getWidth()/divisor, source->getHeight()/divisor,name), name(name), source_divisor(divisor)
{

}


OGLRenderStage::~OGLRenderStage()
{

}

void OGLRenderStage::checkFBOSize()
{
	if (source->sameSize(fbo, source_divisor) == false)
	{
		fbo.resize(source->getWidth() / source_divisor, source->getHeight() / source_divisor);
	}

}

void OGLRenderStage::debugDisplay()
{
	fbo.debugDraw();
}

OGLFrameBuffer * OGLRenderStage::getFBO()
{
	return &fbo;
}

std::string OGLRenderStage::getName()
{
	return name;
}

void OGLRenderStage::setSource(std::shared_ptr<OGLTexture> _source)
{
	source = _source;
}

void OGLRenderStage::InitSharedResources()
{
	static const GLfloat co_buffer_data[] = { -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
	static const GLfloat fs_buffer_data[] = { 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f };

	auto makeVao = [](GLuint& vao, GLfloat const* vtx_data,size_t size) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		GLuint vtx_buffer;
		glGenBuffers(1, &vtx_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vtx_buffer);
		glBufferData(GL_ARRAY_BUFFER, size, vtx_data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);
	};

	makeVao(vao_fs_quad, fs_buffer_data, sizeof(fs_buffer_data));
	makeVao(vao_center_origin_quad, co_buffer_data, sizeof(co_buffer_data));
}
