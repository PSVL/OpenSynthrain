#include "RenderStageRefract.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <random>
#include <algorithm>
#include "imgui/imgui.h"

static const GLfloat vtx_buffer_data[] = {
	0.f, 0.f,
	1.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
};

#ifndef NO_CUDA
extern "C" void sort_pixels(size_t num_pixels);
extern "C" void register_buffer(GLuint buffer);
#endif

RenderStageRefract::RenderStageRefract(std::shared_ptr<OGLTexture> source) : OGLRenderStage(source, 1, "Refract"), refract_shader("./shaders/basic.vert", "./shaders/refract.frag"), bokeh_data{ 0 }
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

	refract_shader.activate();
	GLint tex_uniform = glGetUniformLocation(refract_shader.getProgram(), "bg_tex");
	glUniform1i(tex_uniform, 0);
	tex_uniform = glGetUniformLocation(refract_shader.getProgram(), "bg_blur_tex");
	glUniform1i(tex_uniform, 1);
	tex_uniform = glGetUniformLocation(refract_shader.getProgram(), "drop_tex");
	glUniform1i(tex_uniform, 2);
	/*GLint screen_uniform = glGetUniformLocation(drop_shader.getProgram(), "inv_half_screen_size");
	glUniform2f(screen_uniform,  2.0f / width , 2.0f / height );*/
	refract_shader.deactivate();
	
	glGenBuffers(1, &bokeh_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bokeh_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(bokeh_data), bokeh_data, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bokeh_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	GLuint counter = 0;
	glGenBuffers(1, &bokeh_counter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, bokeh_counter);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, bokeh_counter);

	GLuint buffer_flags = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &counter, buffer_flags);

	// force cast to unsigned int pointer because I know that it is a buffer of unsigned integers
	void* counter_pointer = glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), buffer_flags);
	assert(counter_pointer != nullptr);
	counter_value = static_cast<unsigned int *>(counter_pointer);
	*counter_value = 0;

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);


#ifndef NO_CUDA
	register_buffer(bokeh_ssbo);
#endif
}


RenderStageRefract::~RenderStageRefract()
{
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glDeleteBuffers(1, &bokeh_counter);
	glDeleteBuffers(1, &bokeh_ssbo);
}

void RenderStageRefract::draw(OGLRenderStage *previous_stage)
{
	num_bokeh = 0;

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
	glBindTexture(GL_TEXTURE_2D, source->getHandle());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, previous_stage->getFBO()->getHandle());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bokeh_ssbo);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, bokeh_counter);

	{
		// Equavlient to:
		// glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &num_bokeh);
		// without copying from CPU to GPU

		GLuint zero = 0;
		glInvalidateBufferData(bokeh_counter);
		glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
	}
	

	refract_shader.activate();
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	glBindVertexArray(0);
	refract_shader.deactivate();

	
	//Copy the number of bokeh slient side and read in locally
	/*glBindBuffer(GL_COPY_READ_BUFFER, bokeh_counter);
	glBindBuffer(GL_COPY_WRITE_BUFFER, bokeh_counter_local);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(GLuint));
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, bokeh_counter_local);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &num_bokeh);*/
	
	num_bokeh = std::clamp(*counter_value, 0U, MAX_BOKEH);

#ifdef NO_CUDA
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bokeh_data), &bokeh_data);
	std::sort(bokeh_data, bokeh_data + num_bokeh);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bokeh_data), bokeh_data);
#else
	sort_pixels(num_bokeh);
#endif

	fbo.deactivate();
}

GLuint RenderStageRefract::getBokehCount()
{
	return num_bokeh;
}

GLuint RenderStageRefract::getBokehBuffer()
{
	return bokeh_ssbo;
}

void RenderStageRefract::debugDisplay()
{
	if (ImGui::TreeNode("Shaders"))
	{
		ShowShaderUI(refract_shader)
		ImGui::TreePop();
	}
}
