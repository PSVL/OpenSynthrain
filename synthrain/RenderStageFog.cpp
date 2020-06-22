#include "RenderStageFog.h"
#include "imgui/imgui.h"
#include <sstream>
#include "rng.h"

static const GLfloat fs_vtx_buffer_data[] = {
	0.f, 0.f,
	1.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
};

RenderStageFog::RenderStageFog(std::shared_ptr<OGLTexture> source, std::shared_ptr<OGLFloatTexture> depth_source) :
	OGLRenderStage(source, 1, "Fog"),
	depth(depth_source),
	blur_buffer(source->getWidth(), source->getHeight(), "blur buffer"),
	blur_shader("./shaders/basic.vert", "./shaders/blur.frag"),
	scatter_shader("./shaders/basic.vert", "./shaders/scatter.frag"),
	fog_shader("./shaders/basic.vert", "./shaders/fog.frag"),
	max_blur_radius(3.3f),
	fog_near(0.063),
	fog_far(0.000),
	density(0.23f),
	scattering(0.53),
	fog_color{0.8f,0.8f,0.8f}
{
//	fbo.addDepthBuffer(depth_source);

	depth_blur_buffer.initializeWithFloat(source->getWidth(), source->getHeight(), "depth buffer");

	shaders_dirty = true;

	glGenVertexArrays(1, &bg_vao);
	glBindVertexArray(bg_vao);
	GLuint vtx_buffer;
	glGenBuffers(1, &vtx_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vtx_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fs_vtx_buffer_data), fs_vtx_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindVertexArray(0);

	
	u_blur_direction = glGetUniformLocation(blur_shader.getProgram(), "direction");

	u_fog_radius = glGetUniformLocation(scatter_shader.getProgram(), "fog_max");
	u_scatter_direction = glGetUniformLocation(scatter_shader.getProgram(), "direction");
	u_res = glGetUniformLocation(scatter_shader.getProgram(), "resolution");

	u_alpha = glGetUniformLocation(fog_shader.getProgram(), "alpha");
	u_fog_color = glGetUniformLocation(fog_shader.getProgram(), "fog_color");

	u_density[0] = glGetUniformLocation(fog_shader.getProgram(), "density");
	u_density[1] = glGetUniformLocation(scatter_shader.getProgram(), "density");

	u_fog_near[0] = glGetUniformLocation(fog_shader.getProgram(), "near_fog_plane");
	u_fog_near[1] = glGetUniformLocation(scatter_shader.getProgram(), "near_fog_plane");

	u_fog_far[0] = glGetUniformLocation(fog_shader.getProgram(), "far_fog_plane");
	u_fog_far[1] = glGetUniformLocation(scatter_shader.getProgram(), "far_fog_plane");

	scatter_shader.activate();
	glUniform2f(u_res, fbo.getWidth(), fbo.getHeight());
	scatter_shader.deactivate();
}

RenderStageFog::~RenderStageFog()
{
}

void RenderStageFog::draw(OGLRenderStage * previous_fbo)
{
	OGLFrameBuffer* read_fbo = &blur_buffer;
	OGLFrameBuffer* write_fbo = &fbo;
	GLuint read_tex = source->getHandle();
	
	
	glEnable(GL_CULL_FACE);
	glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glFrontFace(GL_CCW);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depth->getHandle());

	glBindVertexArray(vao_fs_quad);

	blur_shader.activate();
	depth_blur_buffer.activate();
	glUniform2f(u_blur_direction, 1.f, 0.f);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	glUniform2f(u_blur_direction, 0.f, 1.f);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	depth_blur_buffer.deactivate();
	blur_shader.deactivate();


	glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, depth->getHandle());
	glBindTexture(GL_TEXTURE_2D, depth_blur_buffer.getHandle());
	glActiveTexture(GL_TEXTURE0);

//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);




	scatter_shader.activate();
		
	if (shaders_dirty)
	{
		glUniform1f(u_density[1], density);
		glUniform1f(u_fog_near[1], fog_near);
		glUniform1f(u_fog_far[1], fog_far);
	}

	int iterations = 6;
	float radius_scale = 1.f/iterations;

	for (int i = 0; i < iterations; ++i)
	{
		write_fbo->activate();
		
		glBindTexture(GL_TEXTURE_2D, read_tex);


		if (i % 2 == 0)
		{
			glUniform2f(u_scatter_direction, 0, (iterations - i)*max_blur_radius*radius_scale);
		}
		else {
			glUniform2f(u_scatter_direction, (iterations - i)*max_blur_radius*radius_scale,0);
		}

		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
		
		write_fbo->deactivate();

		std::swap(read_fbo, write_fbo);
		read_tex = read_fbo->getHandle();
	}

	scatter_shader.deactivate();

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, blur_buffer.getHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source->getHandle());

	fbo.activate();
	fog_shader.activate();

	if (shaders_dirty)
	{
		glUniform1f(u_density[0], density);
		glUniform1f(u_fog_near[0], fog_near);
		glUniform1f(u_fog_far[0], fog_far);
		glUniform1f(u_alpha, 1 - scattering);
		glUniform3f(u_fog_color, fog_color[0], fog_color[1], fog_color[2]);
		shaders_dirty = false;
	}

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
	fog_shader.deactivate();
	fbo.deactivate();

	glBindVertexArray(0);

	
}

void RenderStageFog::debugDisplay()
{
	ImGui::Text("Parameters");


	if (ImGui::Button("Randomize")) {
		randomizeParameters();
		shaders_dirty = true;
	}

	ImGui::Separator();

	ImGui::DragFloat("Max Blur Radius", &max_blur_radius, 0.1f, 0.f, 30.f);

	if (ImGui::SliderFloat("Density", &density, 0.f, 1.f)) {
		shaders_dirty = true;
	}

	if (ImGui::SliderFloat("Scattering", &scattering, 0.f, 1.f)) {
		shaders_dirty = true;
	}

	if (ImGui::SliderFloat("Fog near cutoff", &fog_near, 0.f, 0.1f)) {
		shaders_dirty = true;
	}

	if (ImGui::SliderFloat("Fog far cutoff", &fog_far, 0.f, 0.1f)) {
		shaders_dirty = true;
	}

	if (ImGui::ColorEdit3("Fog Color", fog_color))
	{
		shaders_dirty = true;
	}

	ImGui::Separator();
	
	if (ImGui::TreeNode("Shaders"))
	{
		ShowShaderUI(fog_shader);
		ShowShaderUI(scatter_shader);
		ShowShaderUI(blur_shader);
		ImGui::TreePop();
	}
}

void RenderStageFog::randomizeParameters()
{
	static std::uniform_real_distribution<float> rng_fog_far(0.000f, 0.017f);
	static std::uniform_real_distribution<float> rng_fog_near(0.f, 0.0800f);
	static std::uniform_real_distribution<float> rng_density(0.1f, 1.000f);
	static std::uniform_real_distribution<float> rng_scattering(0.0f, 1.00f);
	static std::uniform_real_distribution<float> rng_blur(0.0f, 8.00f);
	static std::uniform_real_distribution<float> rng_color_balance(-0.05, 0.05f);


	//fog_far = rng_fog_far(RNG::mt);
	fog_near = fog_far + 0.02f + rng_fog_near(RNG::mt);
	
	

	density = rng_density(RNG::mt);
	scattering = rng_scattering(RNG::mt);
	max_blur_radius = rng_blur(RNG::mt);
	
	float balance = rng_color_balance(RNG::mt);
	fog_color[0] = 0.84 - balance;
	fog_color[1] = 0.84 - balance;
	fog_color[2] = 0.84 + balance;
	shaders_dirty = true;
}
