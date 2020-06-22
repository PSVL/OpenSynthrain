#pragma once
#include <string>
#include <GL/glew.h>
#include <GL/gl.h>
#include <map>
#include <vector>

#define ShowShaderUI(shader) shader.showUI(#shader);

class OGLShader
{
public:
	OGLShader(std::string vtx_file, std::string frag_file, std::string geom_file = "");
	~OGLShader();

	void reloadShaders();
	void activate();
	void debug_uniforms();
	void deactivate();

	GLuint getProgram();

	void showUI(const char* shader_name);

protected:

	static std::vector<char> loadFile(const char* path);
	static GLuint compileShader(GLenum type, const char* name, const char* data);

	OGLShader();
	std::string program_name;

	GLuint program;

	struct unform_info
	{
		GLint position;
		GLint size;
		GLenum type;
		float scale;
	};

	std::string vtx_file;
	std::string frag_file;
	std::string geom_file;

	std::map<std::string,unform_info> uniforms; //should replace this with json.hpp

};

