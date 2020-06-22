#include "Exporter.h"

#include <algorithm>
#include <thread>
#include <mutex>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "OGLTexture.h"


Exporter::Exporter()
{
	quit = false;
	std::generate(exporters, exporters + thread_count, [this]() {return std::thread(std::bind(&Exporter::output_images, this)); });
}

void Exporter::shutdown()
{
	{
		std::lock_guard<std::mutex> lock(mx);
		quit = true;
	}

	export_notify.notify_all();
	for (auto &exporter : exporters) exporter.join();
}

void Exporter::enqueue(fs::path outfile, std::list<OGLImageLike*> textures)
{
	img_save_data out;
	std::string outname;

	std::transform(textures.begin(), textures.end(), std::back_inserter(out.images), [](OGLImageLike* i) {return i->getAsSurface(); });
	out.combine = textures.size() > 1;
	out.name = outfile.string();

	{
		std::unique_lock<std::mutex> lock(mx);
		export_notify.wait(lock, [this] { return output_queue.size() < 100; });
		output_queue.push(out);
	}

	export_notify.notify_all();
}

void Exporter::output_images() {
	static int active_threads = 0;
	static const unsigned int amask = 0xFF000000;
	static const unsigned int bmask = 0x00FF0000;
	static const unsigned int gmask = 0x0000FF00;
	static const unsigned int rmask = 0x000000FF;
	int thread_id = active_threads++;

	while (true) {
		std::unique_lock<std::mutex> lock(mx);
		export_notify.wait(lock, [this] { return quit == true || output_queue.empty() == false; });
		if (quit) break;

		img_save_data out = output_queue.front();
		output_queue.pop();
		lock.unlock();

		if (out.combine == false)
		{
			for (auto img : out.images)
			{
				IMG_SavePNG(img, out.name.c_str());
				//	SDL_FreeSurface(out.clean);
				SDL_FreeSurface(img);
				//	SDL_FreeSurface(out.ground_truth);
			}
		}
		else {

			int totalw = 0;
			int maxh = 0;

			for (auto img : out.images)
			{
				totalw += img->h;
				maxh = std::max(maxh, img->h);
			}

			SDL_Surface* output = SDL_CreateRGBSurface(0, totalw, maxh, 32, rmask, gmask, bmask, amask);
			int left = 0;

			for (auto img : out.images)
			{
				SDL_Rect dest = { left,0,img->w,img->h };
				left += img->w;
				SDL_BlitSurface(img, NULL, output, &dest);
				SDL_FreeSurface(img);
			}

			IMG_SavePNG(output, out.name.c_str());
			SDL_FreeSurface(output);
		}
	}
}

bool Exporter::export_queue_empty()
{
	std::unique_lock<std::mutex> lock(mx);
	return output_queue.empty();
}
