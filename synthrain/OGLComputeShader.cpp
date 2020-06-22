#include "OGLComputeShader.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "SDL2/SDL_log.h"

#include "imgui/imgui.h"

OGLComputeShader::OGLComputeShader(std::string file)
{
	std::vector<char> comp_shader_code = loadFile(file.c_str());

	if (comp_shader_code.size() <= 0) return;

	GLuint comp_shader = compileShader(GL_COMPUTE_SHADER, file.c_str(), &comp_shader_code[0]);

	printf("Linking program\n");
	program = glCreateProgram();
	glAttachShader(program, comp_shader);
	glLinkProgram(program);

	GLint compile_result = GL_FALSE;
	int compile_log_len;

	glGetProgramiv(program, GL_LINK_STATUS, &compile_result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &compile_log_len);
	if (compile_log_len > 0) {
		std::vector<char> errmsg(compile_log_len + 1);
		glGetProgramInfoLog(program, compile_log_len, NULL, &errmsg[0]);
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Shader link failed:\n%s", &errmsg[0]);
	}

	glDetachShader(program, comp_shader);
	glDeleteShader(comp_shader);

	//grab uniforms
	{
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
				float value;
				glGetUniformfv(program, i, &value);
				out.scale = value <= 0.01f ? 0.0001 : value <= 0.1f ? 0.001f : value <= 1.f ? 0.01f : 0.1f;
			}
			uniforms[name] = out;
		}
	}

	program_name = file;
}

OGLComputeShader::~OGLComputeShader()
{
}
