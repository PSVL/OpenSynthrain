#pragma once
#include "OGLTexture.h"
#include <string>
#include <memory>

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct SwsContext;
struct AVPacket;

class OGLVideoTexture : public OGLTexture
{
public:
    static void InitVideo(); // init ffmpeg

	OGLVideoTexture(std::string filepath);
    ~OGLVideoTexture();

	void reload(std::string filepath);
    void next_frame(); 

protected:
	AVPacket *pkt;
	AVFrame *frame;
    AVFormatContext* fmt_ctx;
    AVCodecContext* codec_ctx;
    SwsContext *sws_ctx;
	
//    AVFrame *pFrameRGB;
 
    std::shared_ptr<uint8_t> frame_buffer;
};

