#include "RenderStageDrops.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <array>
#include <iterator>
#include <algorithm>
#include <numeric>

#include "rng.h"
#include "imgui/imgui.h"

static const GLfloat vtx_buffer_data[] = {
	-0.5f, -0.5f,
	0.5f, -0.5f,
	-0.5f, 0.5f,
	0.5f, 0.5f,
};


//first experiment
//static const RenderStageDrops::drop_gen_params def = { 12,8,103,41,1,1 };
//second experiment
static const RenderStageDrops::drop_gen_params def1 = { 15,1,20,0.25,1,1 };
static const RenderStageDrops::drop_gen_params def2 = { 400,1,6,0.5,1,1 };

RenderStageDrops::RenderStageDrops(std::shared_ptr<OGLTexture> source, std::shared_ptr<FlowData> flow_source) :
		OGLRenderStage(source, 1, "Raindrops"), raindrop("./shaders/drop.png"), drop_shader("./shaders/drop.vert", "./shaders/drop.frag"), /*adhesion(source->getWidth(),source->getHeight()),*/ flow(flow_source)
{	
	flow_multiplier = 1;
	num_drops = 0;
	scroll_drops = false;
	physics_frame = false;

	addDrops(def2);
	addDrops(def1);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vtx_buffer;
	glGenBuffers(1, &vtx_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vtx_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_buffer_data), vtx_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 0); //Always use the same set of data for channel 0 (vertices)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glGenBuffers(1, &drop_xform);
	glBindBuffer(GL_ARRAY_BUFFER, drop_xform);
	glBufferData(GL_ARRAY_BUFFER, max_drops * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1); //Use 1 set of data (position etc) per run of channel 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);

	drop_shader.activate();
	screen_uniform = glGetUniformLocation(drop_shader.getProgram(), "inv_half_screen_size");
	glUniform2f(screen_uniform,  2.0f / fbo.getWidth() , 2.0f / fbo.getHeight());
	drop_shader.deactivate();
}


RenderStageDrops::~RenderStageDrops()
{
}

void RenderStageDrops::draw(OGLRenderStage* previous_stage)
{
	if (physics_frame)
	{
		std::uniform_real_distribution<float> uni(0, 1.0);

		for (int i = 0; i < num_drops; ++i)
		{
			auto& x = drops[i].x;
			auto& y = drops[i].y;
			auto& xs = drops[i].sizex;
			auto& ys = drops[i].sizey;


			if (x - xs > this->source->getWidth() || x + xs < 0 ||
				y - ys > this->source->getWidth() || y + ys < 0 )
			{
				x = uni(RNG::mt) * this->source->getWidth();
				y = uni(RNG::mt) * this->source->getHeight();
				physic_drops.x[i] = drops[i].x;
				physic_drops.y[i] = drops[i].y;
			}
		}

		processFlow();

		for (int i = 0; i < num_drops; ++i)
		{
			if (physic_drops.adhesion[i] <= 0)
			{
				fvector2 natural_direction{ (physic_drops.x[i] / source->getWidth()) - 0.5,  -1 };
				natural_direction.normalize();

				float base_speed = std::min(physic_drops.mass[i] * flow_speed, 3.f);

				physic_drops.vx[i] = base_speed * (0.3f*(natural_direction.x) + 0.2f*flow_direction.x);
				physic_drops.vy[i] = base_speed * (0.3f*(natural_direction.y) - 0.2f*flow_direction.y);

				if (fvector2{ physic_drops.vx[i], physic_drops.vy[i] }.len() > 0.2f)
				{

				}

				physic_drops.x[i] += physic_drops.vx[i];
				physic_drops.y[i] += physic_drops.vy[i];
			}

			drops[i].x = physic_drops.x[i];
			drops[i].y = physic_drops.y[i];
		}
	}
	
	physics_frame = false;


	fbo.activate();


	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	if (num_drops > 0)
	{
		drop_shader.activate();
		glUniform2f(screen_uniform, 2.0f / fbo.getWidth(), 2.0f / fbo.getHeight());
		glBindBuffer(GL_ARRAY_BUFFER, drop_xform);
		glBufferData(GL_ARRAY_BUFFER, max_drops * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // disown old buffer
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_drops * sizeof(GLfloat) * 4, drops);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, raindrop.getHandle());
		//update buffers
		glBindVertexArray(vao);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_drops);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_drops);
		glBindVertexArray(0);

		drop_shader.deactivate();
	}

	fbo.deactivate();

}



void RenderStageDrops::debugDisplay()
{

	static bool move_flow = true;
	

	ImGui::Text("Optical Flow Movement");
	ImGui::Checkbox("flow enabled", &move_flow);


	if (move_flow && flow != nullptr)
	{
		auto flowdata = flow->getFlowData();

		//ImGui::BeginChild("Flow");
		ImGui::DragFloat("flow multiplier", &flow_multiplier, 0.01, 0, 100);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvas_size(96.0f, 96.0f);

		draw_list->AddRectFilledMultiColor(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(50, 50, 50, 255), IM_COL32(50, 50, 60, 255), IM_COL32(60, 60, 70, 255), IM_COL32(50, 50, 60, 255));
		draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(255, 255, 255, 255));
		ImGui::InvisibleButton("canvas", canvas_size);

		draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);      // clip lines within the canvas (if we resize it, etc.)

		for (int i = 0; i < flowdata.size(); i += 1)
		{
			int x = i % 3;
			int y = i / 3;
			ImVec2 center(canvas_pos.x + (x * 32) + 16, canvas_pos.y + (y * 32) + 16);
			ImVec2 direction(center);
			direction.x += flowdata[i].x;
			direction.y += flowdata[i].y;

			draw_list->AddLine(center, direction, IM_COL32(255, 255, 0, 255), 2.0f);
		}
		draw_list->PopClipRect();

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::Columns(3);
		static bool selected[16] = { 0 };
		for (int i = 0; i < flowdata.size(); i += 1)
		{
			if (ImGui::GetColumnIndex() == 0) ImGui::Separator();
			ImGui::Text("%7.3f,%7.3f", flowdata[i].x, flowdata[i].y);
			ImGui::NextColumn();
		}
		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::EndGroup();

		if (flow_direction.y < 0)
		{
			ImGui::Text("BACKWARDS\n Speed: %f\nDirection: (%f,%f)", flow_speed, flow_direction.x, flow_direction.y);
		}
		else {
			ImGui::Text("FORWARDS\n Speed: %f\nDirection: (%f,%f)", flow_speed, flow_direction.x, flow_direction.y);
		}
	}

}

void RenderStageDrops::clearDrops()
{
	num_drops = 0;
}

void RenderStageDrops::addDrops(drop_gen_params params)
{
	std::uniform_real_distribution<float> uni(0, 1.0);
	std::lognormal_distribution<float> norm_size{ 1, params.size_dev };
	std::normal_distribution<float> norm_count{ 2, 0.7f*(float)params.count_deviation };

	size_t new_drops = std::round((float)params.count_avg*norm_count(RNG::mt));

	if (new_drops > 3*params.count_avg ) {
		new_drops = 3*params.count_avg;
	}

	if (new_drops + num_drops > max_drops)new_drops = max_drops - num_drops;

	for (int i = num_drops; i < num_drops+ new_drops; ++i)
	{
		drops[i].x = uni(RNG::mt) * this->source->getWidth();
		drops[i].y = uni(RNG::mt) * this->source->getHeight();
		drops[i].sizey = params.size_avg*norm_size(RNG::mt);
		drops[i].sizex = drops[i].sizey*0.8f + 0.2*uni(RNG::mt);

		physic_drops.x[i] = drops[i].x;
		physic_drops.y[i] = drops[i].y;
		physic_drops.vx[i] = 0;
		physic_drops.vy[i] = 0;
		physic_drops.adhesion[i] = 0;
		physic_drops.mass[i] = pow(drops[i].sizey/100.f, 2);
	}
	num_drops += new_drops;
}


void RenderStageDrops::tickPhysics()
{
	physics_frame = true;
}

void RenderStageDrops::processFlow()
{
	auto flowdata = flow->getFlowData();

	int usable_samples = 0;

	for (auto& vec : flowdata)
	{
		if (vec.lenSq() < 4 * 4)
		{
			vec = fvector2{ 0 };
		}
		else {
			++usable_samples;
		}
	}

	// could dot the vectors used with speeds against the direction of their quadrant
	// maybe better for determining forward and/or backward motion
	std::vector<float> speeds;
	std::transform(flowdata.begin(), flowdata.end(), std::back_inserter(speeds),
		[](fvector2 v) -> float { return v.len(); });

	/*	if (usable_samples > 0)
		{
			float avgmag = std::accumulate(speeds.begin(), speeds.end(), 0) / usable_samples;

			for (auto& vec : flowdata)
			{
				if (vec.lenSq() > avgmag2*avgmag2 * 3)
				{
					vec = fvector2{ 0 };
					--usable_samples;
				}
			}

			if (usable_samples > 0)
			{
				avg = std::accumulate(flowdata.begin(), flowdata.end(), fvector2{ 0 }) / usable_samples;
			}
			else {
				avg = fvector2{ 0 };
			}
		}*/


	flow_speed = usable_samples > 0 ? flow_multiplier*std::accumulate(speeds.begin(), speeds.end(), 0) / usable_samples : 0;
	fvector2 bottom_avg{ 0 };

	for (int i = 6; i < 9; ++i) if (speeds[i] > 0) bottom_avg += (flowdata[i] / speeds[i]);

	flow_direction = bottom_avg.len() > 0 ? bottom_avg / 3 : fvector2{ 0 };
}
