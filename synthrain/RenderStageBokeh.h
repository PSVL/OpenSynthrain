#pragma once
#include "OGLRenderStage.h"
#include "OGLTexture.h"
#include "OGLShader.h"

enum BokehShape
{
	Bokeh_Hex = 0,
	Bokeh_Round,
	Bokeh_Max
};

class RenderStageBokeh:
	public OGLRenderStage
{
public:

	RenderStageBokeh(std::shared_ptr<OGLTexture> source);
	virtual ~RenderStageBokeh();
	void draw(OGLRenderStage *previous_fbo);
	void debugDisplay();
	void setBokehShape(BokehShape shape);
	void setFocus(float focus);


private:
	void computeFocus();

	OGLTexture bokeh_billboard[2];
	BokehShape current_billboard;

	OGLShader trivial_shader;
	OGLShader bokeh_shader;
	OGLShader bokeh_billboard_shader;
	
	GLint screen_uniform;

	GLint u_size;
	GLint u_alpha_max;
	GLint u_alpha_min;

	GLuint vao;

	GLuint bokeh_ssbo;

	float focus;
	float brightness;
	bool focus_dirty;

	bool use_billboard;
};


