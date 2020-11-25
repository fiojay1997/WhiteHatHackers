extern "C"
{
// formatter
#include "libavformat/avformat.h"
// decoder
#include "libavcodec/avcodec.h"
// sizing
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
// filter
#include "libavfilter/avfilter.h"
};

#include<iostream>

void Init()
{
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_ERROR);
}

AVFrame* OpenImage(const char* filename)
{
    AVFormatContext *format_ctx = NULL;

    int error = avformat_open_input(&format_ctx, filename, NULL, NULL);
    if(error != 0)
      std::cout << "Error Opening File to Format Context" << std::endl;


    // dump information about file onto standard error                                                                     
    av_dump_format(format_ctx, 0, filename, 0);

    AVCodecContext *codec_ctx = NULL;

    codec_ctx = format_ctx->streams[0]->codec;
    codec_ctx->pix_fmt = AV_PIX_FMT_RGB24;

    // Find the decoder for the video stream
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec)
    {
      std::cout << "Codec not found\n" << std::endl;
        return NULL;
    }

    // Open codec
    if(avcodec_open2(codec_ctx, codec, NULL)<0)
    {
      std::cout << "Could not open codec\n" << std::endl;
        return NULL;
    }

    AVFrame *frame;
    frame = av_frame_alloc();

    if (!frame)
    {
        printf("Can't allocate memory for AVFrame\n");
        return NULL;
    }

    int frameFinished;
    int numBytes;

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *) frame, buffer, AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);

    // Read frame
    AVPacket packet;
    int frames = 0;
    while (av_read_frame(format_ctx, &packet) >= 0)
    {
        if(packet.stream_index != 0)
            continue;

        int ret = avcodec_decode_video2(codec_ctx, frame, &frameFinished, &packet);
        if (ret > 0)
        {
            printf("Frame is decoded, size %d\n", ret);
            frame->quality = 4;
            return frame;
        }
        else
            printf("Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));
    }
}


int main(int argc, char **argv[])
{
    
    // Register all formats and codecs
    Init();

    AVFrame *frame = OpenImage("//myffmpeg/data/Image-00000.JPG");

    AVFormatContext *format_ctx = NULL;
    // Open video file
    int error = avformat_open_input(&format_ctx, "//myffmpeg/data/out.mov", NULL, NULL);
    if(error != 0)
      std::cout << "Error opening output format" << std::endl;

    // Retrieve stream information
    if(avformat_find_stream_info(format_ctx,NULL)<0)
        return -1; // couldn't find stream information
                   // This function populates format_ctx->streams with the proper 

    // dump information about file onto standard error
    av_dump_format(format_ctx, 0, "//myffmpeg/data/out.mov", 0);
    

}
