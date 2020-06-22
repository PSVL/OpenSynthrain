#include "FlowData.h"
#include "SDL2/SDL.h"
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>
#include "imgui/imgui.h"
#include <sstream>
#include <fstream>
#include <vector>

#include <filesystem>
#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

#ifndef _WIN32
#include <libgen.h>
#endif

static int id_num = 0;

FlowData::FlowData() : flow{ 0 }  
{

}

FlowData::FlowData(std::string filepath) :  flow{0}
{
	loadFile(filepath);
}

void FlowData::loadFile(fs::path filepath)
{
	loadFileRW( filepath.filename().string(), SDL_RWFromFile(filepath.string().c_str(), "rb") );
}


void FlowData::loadFileRW(std::string filename, SDL_RWops* io)
{
	const char* path = filename.c_str();

	if (io != NULL) {
		size_t size = SDL_RWsize(io);
		assert(size == sizeof(fvector2)* flow.size());

		if (size > 0)
		{
			if (SDL_RWread(io, flow.data(), size, 1) > 0) {
		//		SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "Loaded flow file: %s", path);
			}
			SDL_RWclose(io);
		}
		else {
			SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "Flow file was empty or invalid: %s", path);
			return;
		}
	}
	else {
		SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "Failed to open flow file: %s", path);
		return;
	}
}

std::array<fvector2, 9> FlowData::getFlowData()
{
	return flow;
}