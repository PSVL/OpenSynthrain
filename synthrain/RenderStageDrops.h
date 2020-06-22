#pragma once
#include "OGLRenderStage.h"
#include "OGLFloatTexture.h"
#include "FlowData.h"
#include "OGLTexture.h"
#include "OGLShader.h"
#include "OGLShader.h"
#include <random>

class RenderStageDrops :
	public OGLRenderStage
{
public:

	struct drop_gen_params
	{
		int count_avg;
		float count_deviation;
		float size_avg;
		float size_dev;
		float gather_strength;
		float gather_iterations;
	};

	RenderStageDrops(std::shared_ptr<OGLTexture> source, std::shared_ptr<FlowData> flow_source);
	virtual ~RenderStageDrops();
	void draw(OGLRenderStage* previous_stage);
	void debugDisplay();
	void clearDrops();
	void addDrops(drop_gen_params params);
	void tickPhysics();

private:

	void processFlow();

	float flow_speed;
	fvector2 flow_direction;

	static const int max_drops = 5000;

	bool scroll_drops;

	struct DropData
	{
		GLfloat x;
		GLfloat y;
		GLfloat sizex;
		GLfloat sizey;
	}drops[max_drops];

	struct DropPhysicsData
	{
		float x[max_drops];
		float y[max_drops];
		float vx[max_drops];
		float vy[max_drops];
		float mass[max_drops];
		float adhesion[max_drops];
	}physic_drops;

	size_t num_drops;
	float flow_multiplier;
	std::shared_ptr<FlowData> flow;
	OGLTexture raindrop;
	//OGLTexture adhesion;

	GLint screen_uniform;
	GLuint vao;
	GLuint drop_xform;
	OGLShader drop_shader;
	bool physics_frame;
	//OGLShader noise_shader;
};
