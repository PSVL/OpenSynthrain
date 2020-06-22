#pragma once
#include "OGLRenderStage.h"
#include "OGLShader.h"
#include "OGLTexture.h"
#include "OGLFloatTexture.h"


class RenderStageFog :
	public OGLRenderStage
{
public:
	RenderStageFog(std::shared_ptr<OGLTexture> source, std::shared_ptr<OGLFloatTexture> depth_source);
	virtual ~RenderStageFog();

	void draw(OGLRenderStage *previous_fbo);
	void debugDisplay();
	void randomizeParameters();

private:	

	std::shared_ptr<OGLFloatTexture> depth;

	OGLFrameBuffer blur_buffer;
	OGLFrameBuffer depth_blur_buffer;

	OGLShader scatter_shader;
	OGLShader blur_shader;
	OGLShader fog_shader;

	float fog_near;
	float fog_far;
	float density;
	float scattering;
	float fog_color[3];

	GLint u_density[2];
	GLint u_fog_near[2];
	GLint u_fog_far[2];
	GLint u_alpha;
	GLint u_fog_color;

	GLint u_fog_radius;
	GLint u_blur_direction;
	GLint u_scatter_direction;
	GLint u_res;

	GLuint bg_vao;

	float max_blur_radius;


	bool shaders_dirty;
};

