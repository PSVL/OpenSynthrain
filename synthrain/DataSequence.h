#pragma once
#include <filesystem>
#include "OGLTexture.h"
#include "OGLFloatTexture.h"
#include "FlowData.h"
#include <map>
#include <queue>
#include <thread>
#include <mutex>

#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif


class DataSequence
{
public:
	DataSequence(std::shared_ptr<OGLTexture> bg_texture, std::shared_ptr<FlowData> flow_texture, std::shared_ptr<OGLFloatTexture> depth_texture);
	~DataSequence();

	void setSequencesFolder(fs::path seq_folder);
	void setSequences(std::vector<std::string> sources);
	void setOutputFolder(fs::path out_folder);

	void setSequence(int idx);
	void advanceSequence();
	
	void setFrame(int idx);
	void advanceFrame();
	
	void setColorFolderName(std::string name);

	bool isEndOfSequence();
	bool isEndOfAllSequences();
	bool useNewParameters();

	void showCacheUI();

	fs::path getCurrentOutputDirectory();
	std::string getCurrentOutputFileName();

	bool showUI();

	int getFileCount();

	int getCurrentFrame(); 

	bool colorDataAvailable();
	bool depthDataAvailable();
	bool flowDataAvailable();

private:
	std::vector<std::string>::iterator sequence_number;
	std::vector<fs::path>::iterator frame_number;

	fs::path base_dir;
	fs::path input_dir_color;
	fs::path input_dir_flow;
	fs::path input_dir_depth;

	fs::path baseoutput_dir;
	fs::path output_dir;

	std::string color_folder_name;

	std::vector<fs::path> color_files;
	std::vector<fs::path> flow_files;
	std::vector<fs::path> depth_files;
	std::vector<std::string> sources;

	std::shared_ptr<OGLTexture> bg_texture;
	std::shared_ptr<FlowData> flow_texture;
	std::shared_ptr<OGLFloatTexture> depth_texture;

	void loadFiles();
	void loadSequence();

	int subsequence_length;

	std::map<fs::path, std::vector<char>> data_cache;

	std::queue<fs::path> load_queue;
	std::mutex mx;
	std::condition_variable load_notify;
	std::thread load_thread;

	static const int front_cache = 8;
	static const int back_cache = 2;
	int cache_misses;

	void load_thread_func();
	void updateCache();
	bool quit;
};

