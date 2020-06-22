#include "OGLVideoTexture.h"
#include "SDL2/SDL.h"
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>
#include "imgui/imgui.h"
#include <sstream>

#ifdef _WIN32
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#else
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#endif


#ifndef _WIN32
#include <libgen.h>
#endif

static int id_num = 0;

#define INBUF_SIZE 4096

void OGLVideoTexture::InitVideo() {
	av_register_all();
}

OGLVideoTexture::OGLVideoTexture(std::string filepath) : OGLTexture()
{
	pkt = av_packet_alloc();
	SDL_assert(pkt != nullptr);
	// Allocate video frame
	frame = av_frame_alloc();
	SDL_assert(frame != nullptr);
	frame_buffer.reset(new uint8_t[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE]);

	reload(filepath);
}

OGLVideoTexture::~OGLVideoTexture()
{
    
}

void OGLVideoTexture::reload(std::string filepath)
{
    SDL_Log("Opening video: %s", filepath.c_str());

	fmt_ctx = avformat_alloc_context();
	SDL_Log("avformat_alloc_context = [%08X]", fmt_ctx);

    int result = avformat_open_input(&fmt_ctx, filepath.c_str(), NULL, NULL);
    SDL_Log("avformat_open_input = %d", result);
    SDL_assert(result == 0);

    result = avformat_find_stream_info(fmt_ctx, NULL);
    SDL_Log("avformat_find_stream_info = %d", result);
    SDL_assert(result >= 0);

	AVCodecParameters* params = nullptr;

    // Find the first video stream
    for(int i = 0; i< fmt_ctx->nb_streams; i++)
    {
        if(fmt_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
			SDL_Log("using video stream %d", i);
			params = fmt_ctx->streams[i]->codecpar;
            break;
        }
    }

	SDL_assert(params != nullptr);
	
    // Get a pointer to the codec context for the video stream
	

	AVCodec *pCodec = nullptr;
	pCodec = avcodec_find_decoder(params->codec_id);
	SDL_Log("avcodec_find_decoder = [%08X]", pCodec);
	SDL_assert(pCodec != nullptr);

    // make our context
	codec_ctx = avcodec_alloc_context3(pCodec);
    result = avcodec_parameters_to_context(codec_ctx, params);
    SDL_Log("avcodec_parameters_to_context = %d", result);
    SDL_assert(result == 0);

    // Open codec
    result = avcodec_open2(codec_ctx, pCodec, NULL);
    SDL_Log("avcodec_open2 = %d", result);
    SDL_assert(result >= 0);
    
    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(codec_ctx->width,
        codec_ctx->height,
        codec_ctx->pix_fmt,
        codec_ctx->width,
        codec_ctx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
        );

    SDL_Log("sws_getContext = %d", sws_ctx);

    internal_format = GL_RGB;
    data_type = GL_UNSIGNED_BYTE;
    data_format = internal_format;

    width = codec_ctx->width;
    height =  codec_ctx->height;

    SDL_Log("Make texture");

    if (handle == 0)
    {
        glGenTextures(1, &handle);
    }

    glBindTexture(GL_TEXTURE_2D, handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  

    glBindTexture(GL_TEXTURE_2D, 0);  
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, data_type, NULL);

    next_frame();
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
	const char *filename)
{
	FILE *f;
	int i;

	fopen_s(&f, filename, "w");
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

void OGLVideoTexture::next_frame()
{
	int frame_no;
	int rcv_result;

	while(true)
	{
		frame_no = av_read_frame(fmt_ctx, pkt);
		SDL_Log("Decoding frame %d", frame_no);

		int result = avcodec_send_packet(codec_ctx, pkt);
		SDL_Log("avcodec_send_packet = %d", result);
		SDL_assert(result <= 0);
		
		result = avcodec_receive_frame(codec_ctx, frame);
		if (result == AVERROR(EAGAIN)) continue;
		if ( result == AVERROR_EOF ) return;

		SDL_Log("avcodec_receive_frame = %d", result);
		SDL_assert(result <= 0);

		SDL_Log("saving frame %3d\n", codec_ctx->frame_number);
		
		/* the picture is allocated by the decoder. no need to
		free it */
		std::ostringstream fname;
		fname << "frame " << codec_ctx->frame_number << ".pgm";
		pgm_save(frame->data[0], frame->linesize[0],
			frame->width, frame->height, fname.str().c_str());
		break;
	}
}