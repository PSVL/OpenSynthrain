#include "RenderStageMaxPool.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <random>
#include "imgui/imgui.h"

static const GLfloat vtx_buffer_data[] = {
	0.f, 0.f,
	1.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
};


RenderStageMaxPool::RenderStageMaxPool(std::shared_ptr<OGLTexture> source) : OGLRenderStage(source, 4, "maxpool"),
											pool_shader("./shaders/basic.vert", "./shaders/pool.frag")
{	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vtx_buffer;
	glGenBuffers(1, &vtx_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vtx_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_buffer_data), vtx_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 0); //Always use the same set of data for channel 0 (vertices)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindVertexArray(0);

	pool_shader.activate();
	GLint tex_uniform = glGetUniformLocation(pool_shader.getProgram(), "tex");
	glUniform1i(tex_uniform, 0);
	pool_shader.deactivate();
	

}


RenderStageMaxPool::~RenderStageMaxPool()
{
}

void RenderStageMaxPool::draw(OGLRenderStage *previous_stage)
{
	fbo.activate();


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);


	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, background.getHandle());



	pool_shader.activate();
	pool_shader.debug_uniforms();
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	glBindVertexArray(0);
	pool_shader.deactivate();


	fbo.deactivate();
}

void RenderStageMaxPool::debugDisplay()
{
	fbo.debugDraw();
}
