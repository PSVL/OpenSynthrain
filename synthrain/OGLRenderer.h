#pragma once
#include <vector>
#include <set>
#include <memory>

#include "RenderStageDrops.h"

class RenderStageFog;;
class RenderStageRefract;
class RenderStageBokeh;
class FlowData;
class OGLTexture;
class OGLFloatTexture;
class OGLRenderStage;

class OGLRenderer
{
public:

	enum class Effect
	{
		Rain,
		Fog,
		Physics
	};

	OGLRenderer(std::shared_ptr<OGLTexture> input_image, std::shared_ptr<FlowData> flow_texture, std::shared_ptr<OGLFloatTexture> depth_texture);
	~OGLRenderer();
	
	void draw();

	void onTick();
	void showSettingsUI();

	void setAvailableEffects(std::set<Effect> eff);

	void setActiveEffects(std::set<Effect> eff);

	void showDropGenerationUI();
	void showStageDebugUI();
	void showSideBySide();
	void setUpRenderChain();
	void prepSequence();

	OGLFrameBuffer* getOutputFBO();

private:
	bool autoplay;

	std::set<Effect> available_effects;
	std::set<Effect> enabled_effects;

	std::shared_ptr<RenderStageFog> fog_stage;
	std::shared_ptr<RenderStageDrops> drop_stage;
	std::shared_ptr<RenderStageRefract> refract_stage;
	std::shared_ptr<RenderStageBokeh> bokeh_stage;

	std::shared_ptr<OGLTexture> color_image;

	static const int num_patterns = 8;
	RenderStageDrops::drop_gen_params noise_layer = { 200,2.0,3.8f,0.33f,1,1 };
	//RenderStageDrops::drop_gen_params noise_layer = { 100,1.0,3.8f,0.33f,1,1 };

	RenderStageDrops::drop_gen_params drop_distribution[num_patterns] = {
		{ 100,1.0f,3.8f,0.33f,1,1 },{ 60,0.125f,1,1.0f,1,1 },{ 150,1.0f,7.835f,0.34f,1,1 },{ 15,1,20,0.25f,1,1 },
	{ 5,0.8f,16,0.55f,1,1 },{ 18,0.2f,15,0.2f,1,1 },{ 12,0.5f,20,0.3f,1,1 },{ 6,0.25f,37,0.3f,1,1 } };

	std::uniform_int_distribution<> dis_noise_layer;
	std::uniform_int_distribution<> dis_bokeh;
	std::uniform_int_distribution<> dis_focus;
	std::uniform_int_distribution<> dis_pattern;
	int selected_drop_layer = 0;

	std::vector < std::shared_ptr<OGLRenderStage>> renderStages;

	//m_renderStages.push_back(fog_stage);
};