#include "OGLShader.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "SDL2/SDL_log.h"

#include "imgui/imgui.h"

OGLShader::OGLShader(std::string vtx_file, std::string frag_file, std::string geom_file) :
	program(0),	vtx_file(vtx_file),	frag_file(frag_file), geom_file(geom_file)
{
	reloadShaders();
}

void OGLShader::reloadShaders()
{
	GLuint old_program = program;

	std::vector<char> vtx_shader_code = loadFile(vtx_file.c_str());
	std::vector<char> frag_shader_code = loadFile(frag_file.c_str());
	std::vector<char> geom_shader_code;

	if (geom_file.size() > 0)
	{
		geom_shader_code = loadFile(geom_file.c_str());
	}

	if (vtx_shader_code.size() <= 0 || frag_shader_code.size() <= 0) return;

	GLuint vtx_shader = compileShader(GL_VERTEX_SHADER, vtx_file.c_str(), &vtx_shader_code[0]);
	GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_file.c_str(), &frag_shader_code[0]);

	printf("Linking program\n");
	program = glCreateProgram();
	glAttachShader(program, vtx_shader);
	glAttachShader(program, frag_shader);
	if (geom_shader_code.size() > 0)
	{
		GLuint geom_shader = compileShader(GL_GEOMETRY_SHADER, geom_file.c_str(), &geom_shader_code[0]);
		glAttachShader(program, geom_shader);
	}

	glLinkProgram(program);

	GLint compile_result = GL_FALSE;
	int compile_log_len;

	glGetProgramiv(program, GL_LINK_STATUS, &compile_result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &compile_log_len);

	glDetachShader(program, vtx_shader);
	glDetachShader(program, frag_shader);

	glDeleteShader(vtx_shader);
	glDeleteShader(frag_shader);

	if (compile_result == GL_FALSE || compile_log_len > 0) {
		std::vector<char> errmsg(compile_log_len + 1);
		glGetProgramInfoLog(program, compile_log_len, NULL, &errmsg[0]);
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Shader link failed:\n%s", &errmsg[0]);
		program = old_program;
	}
	else {
		if(old_program > 0) glDeleteProgram(old_program);

		//grab uniforms
		{
			uniforms.clear();
			GLint i;
			GLint count;

			const GLsizei bufSize = 64;
			GLchar name[bufSize];
			GLsizei name_length;

			glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

			for (i = 0; i < count; i++)
			{
				unform_info out;
				out.position = i;
				glGetActiveUniform(program, (GLuint)i, bufSize, &name_length, &out.size, &out.type, name);
				if (out.type == GL_FLOAT)
				{
					float values[4] = {0};
					glGetUniformfv(program, i, values);
					out.scale = values[0] <= 0.01f ? 0.0001 : values[0] <= 0.1f ? 0.001f : values[0] <= 1.f ? 0.01f : 0.1f;
				}
				uniforms[name] = out;
			}
		}
	}
		
	program_name = vtx_file;
	if (geom_file.size() > 0) program_name += "\n" + geom_file;
	program_name += "\n" + frag_file;
}

void OGLShader::debug_uniforms()
{
	if (ImGui::Begin("Shader Uniforms"))
	{
		if( ImGui::TreeNode(program_name.c_str()) )
		{
			for (auto uniform : uniforms)
			{
				if (uniform.second.type == GL_FLOAT)
				{
					float value;
					glGetUniformfv(program, uniform.second.position, &value);

					if (ImGui::DragFloat(uniform.first.c_str(), &value,uniform.second.scale,0))
					{
						glUniform1f(uniform.second.position, value);
					}
				}
			}

			ImGui::TreePop();
		}
	}
	ImGui::End();

}

OGLShader::~OGLShader()
{
}

void OGLShader::activate()
{
	glUseProgram(program);
}

void OGLShader::deactivate()
{
	glUseProgram(0);
}

GLuint OGLShader::getProgram()
{
	return program;
}

void OGLShader::showUI(const char* shader_name)
{
	if (ImGui::TreeNode(shader_name))
	{
		//ImGui::PushID(shader_name);

		if (ImGui::Button("Reload"))
		{
			reloadShaders();
		}

		if (ImGui::TreeNode("Uniforms")) {
			ImGui::PushItemWidth(120);
			activate();
			for (auto uniform : uniforms)
			{
				if (uniform.second.type == GL_FLOAT)
				{
					float value;
					glGetUniformfv(program, uniform.second.position, &value);

					if (ImGui::DragFloat(uniform.first.c_str(), &value, uniform.second.scale, 0))
					{
						glUniform1f(uniform.second.position, value);
					}
				}
			}
			deactivate();
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::TreePop();		
	}
}

std::vector<char> OGLShader::loadFile(const char* path) {
	SDL_RWops *io = SDL_RWFromFile(path, "rb");
	std::vector<char> out;
	if (io != NULL) {
		size_t size = SDL_RWsize(io);
		if (size > 0)
		{
			out.resize(size + 1);
			if (SDL_RWread(io, &out[0], out.size(), 1) > 0) {
				SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "Loaded shader %s", path);
			}
			SDL_RWclose(io);
			out[size] = 0; //we're not using the string class so we have to null-terminate ourselves.
		}
		else {
			SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "File was empty or invalid for shader %s", path);
		}
	}
	else {
		SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "Failed to open shader %s", path);
	}
	return out;
};

GLuint OGLShader::compileShader(GLenum type, const char* name, const char* data){
	GLuint shader_id = glCreateShader(type);
	GLint compile_result = GL_FALSE;
	int compile_log_len;

	glShaderSource(shader_id, 1, &data, NULL);
	glCompileShader(shader_id);
	// Check Vertex Shader
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_result);
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &compile_log_len);
	if (compile_log_len > 0) {
		std::vector<char> errmsg(compile_log_len + 1);
		glGetShaderInfoLog(shader_id, compile_log_len, NULL, &errmsg[0]);
		SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "Shader compile failed for [%s]:\n%s", name, &errmsg[0]);
	}
	return shader_id;
}

OGLShader::OGLShader()
{
	
}
