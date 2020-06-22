#ifndef NO_CUDA
#ifdef _WIN32
#pragma comment(lib, "cudart_static")
#endif
#endif

#include <iostream>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <iostream>
#include <sstream>
#include <memory>
#include <iomanip>

#include <string>
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

#include <GL/glew.h>
#include <cstring>
#include <stdio.h>
#include <ctime>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC  
#include <crtdbg.h>
#endif

#include <stdlib.h>    

#include "Exporter.h"
#include "LogWindow.h"		
#include "rng.h"
#include "DataSequence.h"

#include <algorithm>
#include <fstream>

#include "OGLFloatTexture.h"
#include "FlowData.h"
#include "json.hpp"

#include "OGLRenderer.h"
#include "OGLRenderStage.h"
#include "RenderStageBokeh.h"

static const int SCREEN_WIDTH = 240;
static const int SCREEN_HEIGHT = 240;

SDL_Surface* surface = nullptr;
SDL_Window* window = nullptr;
SDL_Texture* texture = nullptr;

#ifdef NDEBUG
bool dry_run = false;
#else
bool dry_run = true;
#endif

bool exporting = false;

void ogl_debugmsg(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	SDL_Log("OGL(%d):%s", id , message);
}

int main(int, char**)
{
	{

		srand((unsigned int)time(0));
		SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

		SDL_Log("Rain Simulator started up");

		// Setup SDL
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
		{
			printf("Error: %s\n", SDL_GetError());
			return -1;
		}

		const char* glsl_version = "#version 450";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);


		// Create window with graphics context
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_DisplayMode current;
		SDL_GetCurrentDisplayMode(0, &current);
		window = SDL_CreateWindow("Synthetic Rainmaker", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		SDL_GLContext gl_context = SDL_GL_CreateContext(window);
		SDL_GL_SetSwapInterval(1); // Enable vsync
		glewInit();


		GLint flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		assert(flags & GL_CONTEXT_FLAG_DEBUG_BIT);

		glDebugMessageCallback(ogl_debugmsg, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		//glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,0,nullptr, GL_TRUE);
		//glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,0,nullptr, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);

		// Setup Dear ImGui binding
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

		ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init(glsl_version);

		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

		int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_image could not initialize! SDL_image Error: %s", IMG_GetError());
		}
	
		// Setup style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		OGLRenderStage::InitSharedResources();

		// --- Fundamental init done --- //


		//TODO: App should not crash or throw when there's no data to load
		//TODO: Add data folder config to app with https://github.com/samhocevar/portable-file-dialogs
		//TODO: Move drop generation to a config file
		//TODO: Move automated generation distribution to config file
		//TODO: Update imgui?
		//TODO: Update json++?

		{
			auto input_image = std::make_shared<OGLTexture>();
			auto flow_texture = std::make_shared<FlowData>();
			auto depth_texture = std::make_shared<OGLFloatTexture>(GL_RED);

			DataSequence sequencer(input_image, flow_texture, depth_texture);
			Exporter exporter;
			std::unique_ptr<OGLRenderer> renderer;

			auto load_config = [&]() {

				nlohmann::json config;

				{
					if (std::ifstream cfgfile("./config.json"); cfgfile.good() == true)
					{
						cfgfile >> config;
					}
					else { //make new config
						std::ofstream out_cfgfile("./config.json");
						config["data"]["input_folder"] = "./examples";
						config["data"]["use_sequences"] = { "00" };
						config["data"]["output_folder"] = "./out";

						{
							std::ofstream  cfgfile("config.json");
							if (cfgfile.good())
							{
								cfgfile << std::setw(4) << config << std::endl;
							}
						}
					}
				}

				std::string ifolder = config["data"]["input_folder"];
				std::string ofolder = config["data"]["output_folder"];
				std::vector<std::string> seqs;

				for (auto& e : config["data"]["use_sequences"].items()) {
					seqs.push_back(e.value());
				}

				sequencer.setSequencesFolder(ifolder);
				sequencer.setSequences(seqs);
				sequencer.setOutputFolder(ofolder);
				sequencer.setSequence(0);

				if (sequencer.colorDataAvailable() == true)
				{
					renderer = std::make_unique< OGLRenderer>(input_image, flow_texture, depth_texture);

					if (sequencer.depthDataAvailable() == true)
					{
						renderer->setAvailableEffects({ OGLRenderer::Effect::Fog, OGLRenderer::Effect::Rain });
						renderer->setActiveEffects({ OGLRenderer::Effect::Fog, OGLRenderer::Effect::Rain });
					}
				}

			};

			load_config();

			ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

			// Main loop
			bool done = false;
			bool autoplay = false;

			while (!done)
			{
				SDL_Event event;
				while (SDL_PollEvent(&event))
				{
					ImGui_ImplSDL2_ProcessEvent(&event);
					if (event.type == SDL_QUIT)
						done = true;
					if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
						done = true;
				}

				// Start the ImGui frame
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(window);
				ImGui::NewFrame();

#ifndef NDEBUG
				ImGui::ShowDemoWindow();
#endif

				{
					int w, h;
					SDL_GetWindowSize(window, &w, &h);
					ImGui::SetNextWindowPos(ImVec2(0, 0));
					ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));;
					ImGui::SetNextWindowBgAlpha(1);
				}

				ImGui::Begin("main", NULL, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

				if (renderer == nullptr)
				{
					ImGui::Begin("Locate data");
					
					ImGui::Text("No valid image data at location specified by config file.");
					
					ImGui::Text("Images should be prepared in the following manner:\n"
								"When only color data is present:\n"
								"Place image sequences in sub folders, e.g. \n\\path\\to\\data\\seq1\\\n\\path\\to\\data\\seq1\\\n"
								"In the configuration, \n\"input_folder\" is \\path\\to\\data\\,\n'use_sequences' is [\"seq1\", \"seq2\"]\n"
								"Additionally, when using depth and/or flow data:\n"
								"Place image sequences in sub folders, and divide data into subfolders e.g.\n"
								"\\path\\to\\data\\seq1\\color\\\n"
								"\\path\\to\\data\\seq1\\depth\\\n"
								"\\path\\to\\data\\seq1\\flow\\\n"
					);

					if (ImGui::Button("Open config file"))
					{
						#ifdef _WIN32
							ShellExecute(0, 0, ".\\config.json", 0, 0, SW_SHOW);
						#elif __linux__
							system("xdg-open ./config.json");
						#endif
					}

					if (ImGui::Button("Open config file location"))
					{
						#ifdef _WIN32	
							ShellExecute(0, "open", "explorer.exe", "/select, \".\\config.json\"", 0, SW_SHOW);
						#else
						#endif
					}

					if (ImGui::Button("Reload config file"))
					{
						load_config();
					}

					ImGui::End();
				}else{
					//if (ImGui::Begin("Data Synthesis"), NULL, ImGuiWindowFlags_AlwaysAutoResize)
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.f, 4.f));
					ImGui::BeginGroup();

					if (autoplay == false)
					{
						if (exporting == false)
						{
							ImGui::PushItemWidth(100);

							bool ticked = sequencer.showUI();
							if (ticked == true) renderer->onTick();

							ImGui::PopItemWidth();

							ImGui::Checkbox("Dry run", &dry_run);
							ImGui::SameLine();

							if (ImGui::Button("Begin Synthesis"))
							{
								renderer->prepSequence();
								exporting = true;
								//bokeh_stage->setFocus(0.125f);
							}
						}
						else {

							int counter = sequencer.getCurrentFrame();
							ImGui::Text("Processing %d", counter);
							//ImGui::Text("Sequence No %d/%d", counter/out_seq_len, sequencer.getFileCount()/out_seq_len);

							if (ImGui::Button("Stop Synthesis"))
							{
								exporting = false;
							}
						}

					}

					ImGui::EndGroup();
					ImGui::SameLine();


					ImGui::BeginGroup();
					renderer->showSettingsUI();
					ImGui::EndGroup();

					ImGui::SameLine();
					renderer->showDropGenerationUI();
					ImGui::SameLine();

					renderer->showStageDebugUI();

					ImGui::SameLine();

					{
						static bool show_log = false;
						if (ImGui::Button(show_log ? "Hide Log" : "Show Log", ImVec2(120, 30)))
						{
							show_log = !show_log;
						}

						if (show_log)LogWindow::get()->Draw("Log");
					}

					//ImGui::End();
					ImGui::PopStyleVar();
					ImGui::Spacing();

					renderer->showSideBySide();

					if (autoplay == true) {
						sequencer.advanceFrame();
						renderer->onTick();
						SDL_Delay(1000 / 30);
					}
					else if (exporting == false)
					{
						SDL_Delay(1000 / 60);
					}

					SDL_GL_MakeCurrent(window, gl_context);

					renderer->draw();
					

					if (exporting == true)
					{


						if (dry_run == false)
						{
							fs::path outfile = sequencer.getCurrentOutputDirectory();
							outfile.append(sequencer.getCurrentOutputFileName());
							outfile += ".png";

							exporter.enqueue(outfile, { input_image.get(), renderer->getOutputFBO() });
						}

						sequencer.advanceFrame();
						renderer->onTick();

						if (sequencer.isEndOfSequence() == true)
						{
							sequencer.advanceSequence();

							if (sequencer.isEndOfAllSequences())
							{
								exporting = false;
							}
						}
						else {
							if (sequencer.useNewParameters() == true) renderer->prepSequence();
						}
					}
				}

				ImGui::End();
				// Rendering
				ImGui::Render();

				glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
				glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
				glClear(GL_COLOR_BUFFER_BIT);
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
				SDL_GL_SwapWindow(window);
			}

			exporter.shutdown();


			// Cleanup
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();

			SDL_GL_DeleteContext(gl_context);
			SDL_DestroyWindow(window);
			SDL_Quit();

		}
	}

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

