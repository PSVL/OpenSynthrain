#pragma once
#include "OGLRenderStage.h"
#include "OGLTexture.h"
#include "OGLShader.h"

#define MAX_BOKEH (256000U)

class RenderStageRefract :
	public OGLRenderStage
{
public:
	RenderStageRefract(std::shared_ptr<OGLTexture> source);
	virtual ~RenderStageRefract();
	void draw(OGLRenderStage *previous_fbo);
	void debugDisplay();
	GLuint getBokehCount();
	GLuint getBokehBuffer();
	

private:
	GLuint vao;
	OGLShader refract_shader;

	GLuint bokeh_ssbo;
	GLuint bokeh_counter;
	unsigned int* counter_value;

	GLuint num_bokeh;

	GLuint u_alphaMultiply;
	GLuint u_alphaSubtract;
	GLuint u_minRefraction;
	GLuint u_refractionDelta;
	
	struct bokeh
	{
		float x;
		float y;
		float l;
		float u;
		bool operator<(bokeh &other)
		{
			return l < other.l;
		}
	}bokeh_data[MAX_BOKEH];

};


