#include <vector>
#include <set>
#include <algorithm>
#include <memory>

#include "imgui/imgui.h"

#include "rng.h"

#include "OGLRenderer.h"
#include "OGLTexture.h"
#include "OGLFloatTexture.h"
#include "RenderStageDrops.h"
#include "OGLFrameBuffer.h"
#include "OGLTexture.h"
#include "OGLRenderStage.h"
#include "RenderStageDrops.h"
#include "RenderStageRefract.h"
#include "RenderStageBokeh.h"
#include "RenderStageFog.h"

OGLRenderer::OGLRenderer(std::shared_ptr<OGLTexture> input_image, std::shared_ptr<FlowData> flow_texture, std::shared_ptr<OGLFloatTexture> depth_texture) :
	dis_noise_layer(0, 1),
	dis_bokeh(0, 1),
	dis_focus(3, 8),
	dis_pattern(0, num_patterns - 1)
{
	autoplay = false;

	available_effects = { Effect::Rain };
	enabled_effects = available_effects;

	color_image = input_image;

	fog_stage = std::make_shared<RenderStageFog>(color_image, depth_texture);
	drop_stage = std::make_shared<RenderStageDrops>(color_image, flow_texture);
	refract_stage = std::make_shared<RenderStageRefract>(color_image);
	bokeh_stage = std::make_shared<RenderStageBokeh>(color_image);

	setUpRenderChain();
}

OGLRenderer::~OGLRenderer()
{

}

void OGLRenderer::draw()
{
	for (auto stage : renderStages)
	{
		stage->checkFBOSize();
	}

	OGLRenderStage* last = nullptr;
	for (auto stage : renderStages)
	{
		stage->draw(last);
		last = stage.get();
	}
}

void OGLRenderer::onTick() // do physics simulation stuff
{
	drop_stage->tickPhysics();
}

void OGLRenderer::showSettingsUI()
{
	// Definitely want a better way of doing this if we add any more effects...

	bool usefog = enabled_effects.count(Effect::Fog) > 0;
	bool userain = enabled_effects.count(Effect::Rain) > 0;

	bool dirty = false;
	std::set<Effect> next_effects;

	if (ImGui::Checkbox("Show Rain", &userain))
	{
		dirty = true;
	}

	if (available_effects.count(Effect::Fog) > 0 && ImGui::Checkbox("Show Fog", &usefog))
	{
		dirty = true;
	}

	if (dirty)
	{
		if (userain) next_effects.insert(Effect::Rain);
		if (usefog) next_effects.insert(Effect::Fog);
		setActiveEffects(next_effects);
	}

	/*
	if (exporting == false)
	{
		if (ImGui::Button("Autoplay"))
		{
			autoplay = !autoplay;
		}
	}
	*/
}

void OGLRenderer::setAvailableEffects(std::set<Effect> eff)
{
	available_effects = eff;
	if (available_effects.count(Effect::Rain) == 0) available_effects.insert(Effect::Rain);
	setActiveEffects(enabled_effects); //re-sanitize the enabled effects and update the render chain
}

void OGLRenderer::setActiveEffects(std::set<Effect> eff)
{
	enabled_effects.clear();
	std::set_intersection(eff.begin(), eff.end(), available_effects.begin(), available_effects.end(), std::inserter(enabled_effects, enabled_effects.begin()));
	if (enabled_effects.size() == 0) enabled_effects.insert(Effect::Rain);
	setUpRenderChain();
}

void OGLRenderer::showDropGenerationUI()
{
	if (ImGui::Button("Drop Generator", ImVec2(120, 30)))
	{
		ImGui::OpenPopup("Drop Generator");
	}

	if (ImGui::BeginPopup("Drop Generator"))
	{
		ImGui::SliderInt("Preset", &selected_drop_layer, 0, num_patterns - 1);

		{
			RenderStageDrops::drop_gen_params* params[2] = { &noise_layer, &drop_distribution[selected_drop_layer] };
			int idx = 1;

			for (auto &p : params)
			{
				if (ImGui::TreeNodeEx((void*)idx, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Framed, "Layer %d", idx++))
				{
					ImGui::InputInt("Count", &p->count_avg);
					ImGui::SliderFloat("Count deviation%", &p->count_deviation, 0.01f, 1.f);
					ImGui::SliderFloat("Average Size", &p->size_avg, 0, 80);
					ImGui::SliderFloat("Size deviation%", &p->size_dev, 0.001f, 1.0f);

					if (p->count_avg <= 0) p->count_avg = 1;
					//if (params.count_deviation <= 0) params.count_deviation = 1;
					ImGui::TreePop();
				}
			}
			if (ImGui::Button("Regenerate Drops"))
			{
				drop_stage->clearDrops();
				for (auto p : params)drop_stage->addDrops(*p);
			}
		}
		ImGui::EndPopup();
	}
}

void OGLRenderer::showStageDebugUI()
{
	ImGui::BeginGroup();

	for (auto stage : renderStages)
	{
		std::string namestr = stage->getName();
		const char* name = namestr.c_str();
		if (ImGui::Button(name, ImVec2(120, 30)))
		{
			ImGui::OpenPopup(name);
		}

		if (ImGui::BeginPopup(name))
		{
			stage->debugDisplay();
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		/*ImGui::PushID(name);
		if (ImGui::CollapsingHeader(name))
		{
			stage->debugDisplay();
		}
		ImGui::PopID();*/
	}

	ImGui::EndGroup();
}


void OGLRenderer::showSideBySide()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 4.f));
	ImGui::BeginGroup();
	ImGui::SetCursorPosX(0);

	color_image->debugDraw();

	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::BeginGroup();

	renderStages.back()->getFBO()->getColorBuffer(0)->debugDraw();

	ImGui::PopStyleVar();
	ImGui::EndGroup();
}

void OGLRenderer::setUpRenderChain()
{
	if (enabled_effects.count(Effect::Fog) > 0)
	{
		auto fog_intermediate = fog_stage->getFBO()->getColorBuffer(0);
		drop_stage->setSource(fog_intermediate);
		refract_stage->setSource(fog_intermediate);
		bokeh_stage->setSource(fog_intermediate);
	}
	else {
		drop_stage->setSource(color_image);
		refract_stage->setSource(color_image);
		bokeh_stage->setSource(color_image);
	}

	renderStages.clear();

	if (enabled_effects.count(Effect::Fog) > 0)
	{
		renderStages.push_back(fog_stage);
	}

	if (enabled_effects.count(Effect::Rain) > 0)
	{
		renderStages.push_back(drop_stage);
		renderStages.push_back(refract_stage);
		renderStages.push_back(bokeh_stage);
	}
}

void OGLRenderer::prepSequence() {
	drop_stage->clearDrops();
	if (dis_noise_layer(RNG::mt) == 0)drop_stage->addDrops(noise_layer);
	drop_stage->addDrops(drop_distribution[dis_pattern(RNG::mt)]);
	//drop_stage->addDrops(drop_distribution[selected_drop_layer]);
	bokeh_stage->setBokehShape((BokehShape)dis_bokeh(RNG::mt));
	bokeh_stage->setFocus(0.125f*dis_focus(RNG::mt));
	fog_stage->randomizeParameters();
}

OGLFrameBuffer* OGLRenderer::getOutputFBO()
{
	return renderStages.back()->getFBO();
}

