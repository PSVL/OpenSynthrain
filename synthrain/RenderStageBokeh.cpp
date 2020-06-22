#include "RenderStageBokeh.h"
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

RenderStageBokeh::RenderStageBokeh(std::shared_ptr<OGLTexture> source) :	OGLRenderStage(source, 1, "Defocus"),
									    bokeh_billboard{ OGLTexture("./shaders/bokeh-hex.png"), OGLTexture("./shaders/bokeh-round2.png") },
										bokeh_shader("./shaders/basic.vert", "./shaders/bokeh.frag", "./shaders/bokeh-hex.geom"),
										bokeh_billboard_shader("./shaders/basic.vert", "./shaders/bokeh-billboard.frag", "./shaders/bokeh-billboard.geom"),
										trivial_shader("./shaders/basic.vert", "./shaders/basic.frag")
{	
	focus = 0.4f;
	brightness = 10.f;
	use_billboard = true;
	focus_dirty = false;
	current_billboard = Bokeh_Hex;

	fbo.addColorBuffer();

	bokeh_billboard_shader.activate();
	{
		GLint tex_uniform = glGetUniformLocation(bokeh_billboard_shader.getProgram(), "tex");
		glUniform1i(tex_uniform, 0);

		tex_uniform = glGetUniformLocation(bokeh_billboard_shader.getProgram(), "billboard_tex");
		glUniform1i(tex_uniform, 1);

		screen_uniform = glGetUniformLocation(bokeh_billboard_shader.getProgram(), "u_aspect");
		glUniform1f(screen_uniform, (float)fbo.getWidth() / fbo.getHeight());

		u_size = glGetUniformLocation(bokeh_billboard_shader.getProgram(), "u_size");
		u_alpha_max = glGetUniformLocation(bokeh_billboard_shader.getProgram(), "u_alpha_max");
		u_alpha_min = glGetUniformLocation(bokeh_billboard_shader.getProgram(), "u_alpha_min");
	}
	bokeh_billboard_shader.deactivate();

/*	bokeh_shader.activate();
	{
		GLint tex_uniform = glGetUniformLocation(bokeh_shader.getProgram(), "tex");
		glUniform1i(tex_uniform, 0);

		screen_uniform = glGetUniformLocation(bokeh_shader.getProgram(), "u_aspect");
		glUniform1f(screen_uniform, (float)fbo.getWidth() / fbo.getHeight());
	}
	bokeh_shader.deactivate();*/


	vao = 0;
}

RenderStageBokeh::~RenderStageBokeh()
{
}

void RenderStageBokeh::draw(OGLRenderStage *previous_stage)
{
	RenderStageRefract* refract_stage = dynamic_cast<RenderStageRefract*>(previous_stage);

	if (vao == 0)
{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, refract_stage->getBokehBuffer());
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);
	}

	fbo.activate();
	
	glEnable(GL_CULL_FACE);
	glBlendFunci(0,GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunci(1, GL_ONE, GL_ONE);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glFrontFace(GL_CCW);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source->getHandle());

	glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	trivial_shader.activate();
	glBindVertexArray(vao_fs_quad);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4,1);
	glBindVertexArray(0);
	trivial_shader.deactivate();
	glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, refract_stage->getFBO()->getHandle());


	if (use_billboard == true)
	{
		bokeh_billboard_shader.activate();

		//if (focus_dirty == true)
		{
			computeFocus();
		//	focus_dirty = false;
		}

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bokeh_billboard[current_billboard]);

		glUniform1f(screen_uniform, (float)fbo.getWidth() / fbo.getHeight());
		glBindVertexArray(vao);
		glDrawArrays(GL_POINTS, 0, refract_stage->getBokehCount());
		glBindVertexArray(0);

		bokeh_billboard_shader.deactivate();
	}
	/*else {
		glFrontFace(GL_CW);

		bokeh_shader.activate();

	//	if (focus_dirty == true)
		{
			computeFocus();
		//	focus_dirty = false;
		}

		glUniform1f(screen_uniform, (float)fbo.getWidth() / fbo.getHeight());
		bokeh_shader.debug_uniforms();
		glBindVertexArray(vao);
		glDrawArrays(GL_POINTS, 0, refract_stage->getBokehCount());
		glBindVertexArray(0);
		bokeh_shader.deactivate();
	}*/

	fbo.deactivate();
}

void RenderStageBokeh::debugDisplay()
{
	
	ImGui::Text("Parameters");
	bool is_circle = current_billboard == Bokeh_Round;
	bool is_hex = current_billboard == Bokeh_Hex;

	if (ImGui::RadioButton("Circle", is_circle))
	{
		current_billboard = Bokeh_Round;
	}
	if (ImGui::RadioButton("Hex", is_hex)) {
		current_billboard = Bokeh_Hex;
	}

	if (ImGui::SliderFloat("Focus", &focus, 0.125, 1))
	{
		focus_dirty = true;
	}

	ImGui::SliderFloat("brightness", &brightness, 1, 100);

	ImGui::Separator();
	if (ImGui::TreeNode("Shaders"))
	{
		ShowShaderUI(bokeh_billboard_shader);
		ImGui::TreePop();
	}
}

void RenderStageBokeh::setBokehShape(BokehShape shape)
{
	current_billboard = shape;
}

void RenderStageBokeh::setFocus(float _focus)
{
	focus = std::clamp(_focus, 0.0f, 1.0f);
	focus_dirty = true;
}


void RenderStageBokeh::computeFocus()
{
	auto bezier_lerp = [](float t, float p1, float p2, float p3, float p4)->float{
		float u = 1.0f - t;
		float w1 = u * u*u;
		float w2 = 3 * u*u*t;
		float w3 = 3 * u*t*t;
		float w4 = t * t*t;
		return w1*p1 + w2 * p2 + w3 * p3 + w4 * p4;
	};

	float norm_pixel = 1.f / (source->getWidth());
	float size = norm_pixel + focus * (0.18f - norm_pixel);// 0.01f + focus * (0.18f - 0.01f);
	float radius = ((size * source->getWidth()))*0.4f;
	float area;
	
	switch (current_billboard)
	{
	case Bokeh_Hex:
		area = 6 * sqrt(3) / 4 * (radius*radius);
		break;
	case Bokeh_Round:
	default:
		area = (3.14159265359f*radius*radius);
		break;

	}
	float alpha_max = brightness / area; 		//0.005 + 0.01*(1-focus) + 0.904*exp(focus * -15.0f);
	alpha_max = std::clamp(alpha_max, 0.f, 1.f);

	float alpha_min = alpha_max * 0.8f;// 0.0025 + 0.996*exp(focus * -9.0f);
	alpha_min = std::clamp(alpha_min, 0.f, 1.f);
	

//	if (ImGui::Begin("Bokeh"))

	/*if (ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::BeginChild("Config", ImVec2(400, fbo.getHeight()), true);
		{
			ImGui::Separator();
			ImGui::Text("Focus vars");
			ImGui::InputFloat("Normalized Size", &size, 0, 0, 5, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Pixel Radius", &radius, 0, 0, 5, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Pixel Area", &area, 0, 0, 5, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Alpha Min", &alpha_min, 0, 0, 5, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Alpha Max", &alpha_max, 0, 0, 5, ImGuiInputTextFlags_ReadOnly);
		}
		ImGui::EndChild();
	}
	ImGui::End();*/

	glUniform1f(u_size, size);
	glUniform1f(u_alpha_max, alpha_max);
	glUniform1f(u_alpha_min, alpha_min);
}
