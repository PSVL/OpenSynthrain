#pragma once

#include "fvector2.h"
#include <string>
#include <array>
#include <SDL2/SDL.h>

#include <filesystem>
#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif


class FlowData
{
public:

	FlowData(std::string filepath);
	FlowData();

	void loadFile(fs::path filepath);
	void loadFileRW(std::string filename, SDL_RWops* filehandle);

	std::array<fvector2, 9> getFlowData();

protected:

	std::array<fvector2, 9> flow;
};

