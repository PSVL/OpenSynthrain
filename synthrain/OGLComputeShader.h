#pragma once
#include <string>
#include "OGLShader.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <map>

class OGLComputeShader : public OGLShader
{
public:
	OGLComputeShader(std::string file);
	virtual ~OGLComputeShader();
private:

};

