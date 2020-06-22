#include "DataSequence.h"

#include <filesystem>
#include <algorithm>
#include <thread>
#include <iostream>
#include <queue>
#include <mutex>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

#include "imgui/imgui.h"

DataSequence::DataSequence(std::shared_ptr<OGLTexture> bg_texture, std::shared_ptr<FlowData> flow_texture, std::shared_ptr<OGLFloatTexture> depth_texture) :
	bg_texture(bg_texture),
	flow_texture(flow_texture),
	depth_texture(depth_texture),
	color_folder_name("frames"),
	sources{},
	load_thread(&DataSequence::load_thread_func, this)
{
	sequence_number = sources.begin();
	subsequence_length = 0;
	cache_misses = 0;

	quit = false;
}


DataSequence::~DataSequence()
{
	{
		std::lock_guard<std::mutex> lock(mx);
		quit = true;
	}

	load_notify.notify_all();
	load_thread.join();

}

void DataSequence::setSequencesFolder(fs::path seq_folder)
{
	base_dir = seq_folder;
}

void DataSequence::setSequences(std::vector<std::string> _sources)
{
	sources = _sources;
}

void DataSequence::setOutputFolder(fs::path out_folder)
{
	baseoutput_dir = out_folder;
}

void DataSequence::setSequence(int idx)
{
	sequence_number = sources.begin() + idx;
	loadSequence();
	
}

void DataSequence::advanceSequence()
{
	sequence_number++;
	loadSequence();
}

void DataSequence::setFrame(int idx)
{
	frame_number = color_files.begin() + idx;
	updateCache();
	loadFiles();
}

void DataSequence::advanceFrame()
{ 
	frame_number++;
	if (frame_number == color_files.end()) frame_number = color_files.begin();
	updateCache();
	loadFiles();

}

void DataSequence::setColorFolderName(std::string name)
{
	color_folder_name = name;
}

bool DataSequence::isEndOfSequence()
{
	if (subsequence_length > 0)
	{
		if (std::distance(color_files.begin(), frame_number) + subsequence_length >= color_files.size()) return true;
	}

	return frame_number == color_files.end();
}

bool DataSequence::isEndOfAllSequences()
{
	return sequence_number == sources.end();
}

bool DataSequence::useNewParameters()
{
	return subsequence_length == 0 || (std::distance(color_files.begin(), frame_number) % subsequence_length) == 0;
}

void DataSequence::showCacheUI()
{
	int queue_size;
	int cache_size;
	{
		std::unique_lock<std::mutex> lock(mx);
		queue_size = load_queue.size();
		cache_size = data_cache.size();
	}
	ImGui::Text("Queue Size: %d", queue_size);
	ImGui::Text("Cache Size: %d", cache_size);
	ImGui::Text("Cache misses: %d", cache_misses);
}

fs::path DataSequence::getCurrentOutputDirectory()
{
	return output_dir;
}

std::string DataSequence::getCurrentOutputFileName()
{
	if (subsequence_length > 0)
	{
		std::ostringstream name;
	
		int seq = std::distance(sources.begin(), sequence_number);
		int global_frame = std::distance(color_files.begin(), frame_number);
		int subseq = global_frame / subsequence_length;
		int frame = global_frame % subsequence_length;

		name << std::setfill('0') << std::setw(2) << seq << "_" << std::setw(3) << subseq << "_" << std::setw(4) << frame;
		return name.str();
	}
	else {
		return frame_number->stem().string();
	}
}

bool DataSequence::showUI()
{
	bool changed_frame = false;

	ImGui::PushItemWidth(100);

	int seq = std::distance(sources.begin(), sequence_number);
	if (ImGui::InputInt("Sequence", &seq))
	{
		if (seq <= 0) seq += sources.size();
		seq %= sources.size();
		setSequence(seq);
		changed_frame = true;
	}

	int frame = std::distance(color_files.begin(), frame_number);
	if (ImGui::InputInt("Start from image", &frame))
	{
		if (frame <= 0) frame += color_files.size();
		frame %= color_files.size();
		setFrame(frame);
		changed_frame = true;
	}

	if (ImGui::InputInt("Subsequence length", &subsequence_length))
	{
		if (subsequence_length <= 0) subsequence_length = 0;
		subsequence_length %= 300;
	}

	return changed_frame;
}

int DataSequence::getFileCount()
{
	return color_files.size();
}

int DataSequence::getCurrentFrame()
{
	return std::distance(color_files.begin(), frame_number);
}

bool DataSequence::colorDataAvailable()
{
	return color_files.empty() == false;;
}

bool DataSequence::depthDataAvailable()
{
	return depth_files.empty() == false;
}

bool DataSequence::flowDataAvailable()
{
	return flow_files.empty() == false;
}

void DataSequence::loadFiles()
{
	if (isEndOfSequence() == true) return;

	{
		std::unique_lock<std::mutex> lock(mx);
		if (data_cache[*frame_number].empty() == false)
		{
			auto ops = SDL_RWFromMem(data_cache[*frame_number].data(), data_cache[*frame_number].size());
			bg_texture->loadFileRW(frame_number->filename().string(), ops);
		}
		else {
			lock.unlock();
			bg_texture->loadFile((*frame_number).string());
			cache_misses++;
		}
	}

	int frame_index = getCurrentFrame();

	if (flow_files.empty() == false && frame_index < flow_files.size())
	{
		std::unique_lock<std::mutex> lock(mx);
		auto flow_path = flow_files[frame_index];
		if (data_cache[flow_path].empty() == false)
		{
			auto ops = SDL_RWFromMem(data_cache[flow_path].data(), data_cache[flow_path].size());
			flow_texture->loadFileRW(flow_path.string(), ops);
		}
		else 
		{
			lock.unlock();
			flow_texture->loadFile(flow_path.string());
			cache_misses++;
		}
	}

	if (depth_files.empty() == false && frame_index < depth_files.size())
	{
		std::unique_lock<std::mutex> lock(mx);
		auto depth_path = depth_files[frame_index];
		if (data_cache[depth_path].empty() == false)
		{
			auto ops = SDL_RWFromMem(data_cache[depth_path].data(), data_cache[depth_path].size());
			depth_texture->loadFileRW(depth_path.string(), ops);
		}
		else
		{
			lock.unlock();
			depth_texture->loadFile(depth_path.string());
			cache_misses++;
		}
	}
}

void DataSequence::loadSequence()
{
	if (isEndOfAllSequences() == true) return;
	color_files.clear();
	flow_files.clear();
	depth_files.clear();

	std::string seq_folder = *sequence_number;

	input_dir_color = base_dir;
	input_dir_color.append(seq_folder).append(color_folder_name);

	input_dir_flow = base_dir;
	input_dir_flow.append(seq_folder).append("flows");

	input_dir_depth = base_dir;
	input_dir_depth.append(seq_folder).append("depth");

	output_dir = baseoutput_dir;
	output_dir.append(seq_folder);

	auto filter_func = [](const fs::path& p) {
		auto static const valid = std::set<std::string>{".jpg", ".png", ".jpeg", ".bin", ".flow"};
		return fs::is_regular_file(p) && valid.count(p.extension().string()) > 0;
	};

	if (fs::exists(input_dir_color))
	{
		bool color_only = true;

		fs::directory_iterator diff_iterator = fs::directory_iterator(input_dir_color);

		copy_if(fs::directory_iterator(diff_iterator), fs::directory_iterator(), back_inserter(color_files), filter_func);


		if (fs::exists(input_dir_flow)) {
			fs::directory_iterator flow_iterator = fs::directory_iterator(input_dir_flow);
			copy_if(fs::directory_iterator(flow_iterator), fs::directory_iterator(), back_inserter(flow_files), filter_func);
			color_only = false;
		}


		if (fs::exists(input_dir_depth)) {
			fs::directory_iterator depth_iterator = fs::directory_iterator(input_dir_depth);
			copy_if(fs::directory_iterator(depth_iterator), fs::directory_iterator(), back_inserter(depth_files), filter_func);
			color_only = false;
		}

		if (color_only == true)
		{
			std::sort(color_files.begin(), color_files.end());
		}
	}
	else {
		input_dir_color = base_dir;
		input_dir_color.append(seq_folder);
		fs::directory_iterator diff_iterator = fs::directory_iterator(input_dir_color);
		copy_if(fs::directory_iterator(diff_iterator), fs::directory_iterator(), back_inserter(color_files), filter_func);
	}

	/* //TODO: What does this code do again???
	if (depth_files.size() > 0 && depth_files.size() < color_files.size())
	{
		std::set<fs::path> temp;
		temp.insert(make_move_iterator(color_files.begin()), make_move_iterator(color_files.end()));

		//std::copy(, , std::inserter(temp, temp.end()));

		color_files.clear();

		for (auto depth : depth_files)
		{
			int fileno = 0;
			{
				std::stringstream s(depth.stem().string());
				s >> fileno;
			}

			fileno += 1;
			std::stringstream s;
			s << std::setfill('0') << std::setw(6) << fileno<<".png";
			fs::path colorpath = input_dir_color;
			colorpath.append(s.str());
			color_files.push_back(colorpath);
		}
	}*/


	frame_number = color_files.begin();
	updateCache();

	fs::create_directories(output_dir);

	setFrame(0);
}

void DataSequence::load_thread_func()
{
	while (true) {
		std::unique_lock<std::mutex> lock(mx);
		load_notify.wait(lock, [this] { return quit == true || load_queue.empty() == false; });

		if (quit) break;

		auto path_in = load_queue.front();
		load_queue.pop();
		std::cout << "Loading " << path_in << std::endl;
		lock.unlock();


		std::ifstream file(path_in.string(), std::ios::binary | std::ios::ate);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		assert(size > 0);

		std::vector<char> buf(size);

		if (!file.read(buf.data(), size))
		{
			assert(false);
			//error!
		}

		lock.lock();
		data_cache[path_in] = std::move(buf);
		lock.unlock();
	}
}

void DataSequence::updateCache()
{
	{
	std::lock_guard<std::mutex> lock(mx);

	//auto find_and_cache = [this](std::vector<fs::path>::iterator begin, std::vector<fs::path>::iterator end)
	std::map<fs::path, std::vector<char>> last;
	{
		last = std::move(data_cache);
	}

	auto find_and_cache = [this](std::vector<fs::path> list, std::map<fs::path, std::vector<char>>& temp)
	{
		if (list.empty() == true) return;
		unsigned int idx = std::distance(color_files.begin(), frame_number);
		auto center = list.begin() + idx;
		std::vector<fs::path>::iterator begin = idx >= back_cache ? center - back_cache : list.begin();
		std::vector<fs::path>::iterator end = idx + front_cache < list.size() ? (center + 1 + front_cache) : list.end();
		assert(std::distance(begin, end) <= 1 + front_cache + back_cache);

		{
			for (auto it = begin; it != end; ++it)
			{
				if (temp[*it].empty() == false)
				{
					//	std::cout << "Resuing " << *it << std::endl;
					data_cache[*it] = std::move(temp[*it]);
				}
			}

			/*for (auto it = temp.begin(); it != temp.end(); ++it)
			{
				if (it->second.empty() == false)std::cout << "Forgetting " << it->first.string() << std::endl;
			}*/
		}

		for (auto it = begin; it != end; ++it)
		{
			auto& cached = data_cache[*it];

			if (cached.empty() == true)
			{
				load_queue.push(*it);
				//	std::cout << "Queued " << *it << std::endl;
			}
		}
	};

	find_and_cache(color_files, last);
	find_and_cache(depth_files, last);
	find_and_cache(flow_files, last);
}
	load_notify.notify_all();
	//std::cout << std::endl;
}

