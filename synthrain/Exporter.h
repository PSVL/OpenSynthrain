#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable> 

#include <filesystem>
#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

#include "OGLTexture.h"

class Exporter

{
public:
	static const unsigned int thread_count = 12;
	std::thread exporters[thread_count];

	Exporter();

	void shutdown();

	void enqueue(fs::path outfile, std::list<OGLImageLike*> textures);

	void output_images();

	bool export_queue_empty();

private:
	bool quit;

	struct img_save_data
	{
		std::list<SDL_Surface*> images;
		std::string name;
		bool combine;
	};

	std::mutex mx;
	std::condition_variable export_notify;
	std::queue<img_save_data> output_queue;
};