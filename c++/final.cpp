#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <chrono>

extern "C"
{
#include <string>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

AVFrame * open_file(const char * filename) 
{
    AVFormatContext * format_ctx = NULL;
    int error_code;
    error_code = avformat_open_input(&format_ctx, filename, NULL, NULL);
    if (error_code != 0)
    {
    	std::cout << "Could not open " << filename << "." << std::endl;
	return NULL;
    }
    
    error_code = avformat_find_stream_info(format_ctx, NULL);
    if (error_code != 0)
    {
        std::cout << "Could not find stream info" << std::endl;
	return NULL;
    }
    AVCodec * codec = NULL;
    codec = avcodec_find_decoder(format_ctx->streams[0]->codec->codec_id);
    if (codec == NULL)
    {
    	std::cout << "Could not load decoder" << std::endl;
	return NULL;
    }

    AVCodecContext * codec_ctx = format_ctx->streams[0]->codec;
    error_code = avcodec_open2(codec_ctx, codec, NULL);
    if (error_code != 0)
    {
    	std::cout << "Could not load codec for this file" << std::endl;
    }
    
    AVFrame * frame;
    AVPacket packet;

    frame = av_frame_alloc();
    av_read_frame(format_ctx, &packet);
    
    int finished;
    avcodec_decode_video2(codec_ctx, frame, &finished, &packet);
    std::cout << finished << std::endl;

    AVFrame * rgbFrame = av_frame_alloc();
    rgbFrame->width = frame->width;
    rgbFrame->height = frame->height;
    rgbFrame->format = AV_PIX_FMT_YUV420P; 
    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
    uint8_t * buffer = (uint8_t*) av_malloc(numBytes);
    avpicture_fill((AVPicture*) rgbFrame, buffer, AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
    
    struct SwsContext *sws_ctx;
    sws_ctx = sws_getContext(frame->width, frame->height, (AVPixelFormat) frame->format, frame->width,
		    frame->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL); 
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height, 
		    rgbFrame->data, rgbFrame->linesize);
    
    return rgbFrame;
}

/**
 * Generate output file name
 * The name should conatin the current time
 * The file type is mp4
 */
const char * get_filename()
{
    const char * v_fmt = "mp4";
    std::string filename;
    auto time_now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(time_now);
    std::string current_time_s(std::ctime(&current_time));
    filename = current_time_s + ".mp4";
    return filename.c_str();
}

/**
 * create video stream for saving video 
 * This should pass a AVFormatContext pointer to the saving latter
 */
AVStream * add_vidio_stream(AVFormatContext * oc, enum AVCodecID codec_id)
{
    AVStream * st;
    AVCodec * codec;

    st = avformat_new_stream(oc, NULL);
    if (st == NULL)
    {
        std::cout << "Could not alloc stream" << std::endl;
        return NULL;
    }

    codec = avcodec_find_encoder(codec_id);
    if (codec == NULL)
    {
        std::cout << "Could not load codec" << std::endl;
	return NULL;
    }

    avcodec_get_context_defaults3(st->codec, codec);

    st->codec->bit_rate = 400000;
    st->codec->width = 1080;
    st->codec->height = 1800;
    st->codec->time_base.den = 10;
    st->codec->time_base.num = 1;
    st->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    st->codec->codec_tag = 0;
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    return st;
}

/*
int encode_video(std::vector<AVFrame*> frames, enum AVCodecID codec_id, const char * filename)
{
    AVCodec * codec = NULL;
    AVCodecContext * codec_ctx = NULL;

    avcodec_register_all();
    codec = avcodec_find_encoder(codec_id);
    if (!codec)
    {
        std::cout << "Could find find the encoder" << std::endl;
        return -1; 
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        std::cout << "Could not allocate conetxt" << std::endl;
        return -1;
    }

    codec_ctx->bit_rate = 4000000;
    codec_ctx->width = 1080;
    codec_ctx->height = 1800;
    codec_ctx->time_base = (AVRational) {1, 25};
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 1;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
    
    if (avcodec_open2(codec_ctx, codec, NULL) < 0)
    {
    	std::cout << "Open encoder fialded" << std::endl;
    }    
    
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    int got_packet_ptr = 1;
    for (auto frame : frames)
    {
         int res = avcodec_encode_video2(codec_ctx, &pkt, frame, got_packet_ptr);
	 if (res != 0)
	 {
	     std::cout << "Encoding failed : " << res << std::endl;
	     return -1;
	 }

	 if (got_packet_ptr)
	 {
	     fwrite(pkt.data, 1, pkt.size, filename);
	 }
    }
}
*/


int main()
{
    AVFrame * frame;
    frame = open_file("../pictures/2.jpeg");
    frame2 = open_file("../pictures/3.jpeg");
    frame3 = open_file("../pictures/3.jpeg");

    if (frame == NULL || frame2 == NULL || frame3 == NULL)
    {
    	std::cout << "Generate failed" << std::endl;
	return -1;
    }

    AVCodec * video_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (video_codec == NULL)
    {
        std::cout << "Failed to load encoder" << std::endl;
        return -1;
    }

    AVCodecContext * video_ctx = avcodec_alloc_context3(video_codec);
    if (video_ctx == NULL)
    {
        std::cout << "Failed to load encoder context" << std::endl;
        return -1;
    }

    video_ctx->bit_rate = 4000000;
    video_ctx->width = frame->width;
    video_ctx->height = frame->height;
    video_ctx->time_base = (AVRational) {1, 25};
    video_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;

    if (avcodec_open2(video_ctx, video_codec, NULL) < 0)
    {
        std::cout << "Failed to open encoder" << std::endl;
        return -1;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
    int got_packet_ptr;
    int error_code = avcodec_encode_video2(video_ctx, &packet, frame, &got_packet_ptr);
    if (error_code < 0)
    {
        std::cout << "Failed to encode video" << std::endl;
	return -1;
    }
    
    if (got_packet_ptr)
    {
        std::cout << "load succeed" << std::endl;
	std::ofstream out_file("demo2.mov");
	out_file.write((const char *) packet.data, packet.size);
	out_file.close();
	return 0;
    }
    

    /*
    AVCodec * out_codec = avcodec_find_encoder(AV_CODEC_ID_BMP);
    if (out_codec == NULL)
    {
    	std::cout << "Failed" << std::endl;
	    return -1;
    }

    AVCodecContext * out_ctx = avcodec_alloc_context3(out_codec);
    if (out_ctx == NULL)
    {
    	std::cout << "Failed" << std::endl;
	    return -1;
    }

    out_ctx->pix_fmt = AV_PIX_FMT_BGR24;
    out_ctx->width = frame->width;
    out_ctx->height = frame->height;
    out_ctx->time_base.num = 1;
    out_ctx->time_base.den = 1;

    avcodec_open2(out_ctx, out_codec, NULL);
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
    
    int done;
    int error_code;
    error_code = avcodec_encode_video2(out_ctx, &packet, frame, &done);
    if (error_code != 0)
    {
    	std::cout << "Encode error" << std::endl;
	return -1;
    }

    std::cout << "encoded" << std::endl;
    
    std::ofstream out_file("demo.bpm");
    out_file.write((const char *) packet.data, packet.size);
    out_file.close();
    return 0;
    */


}

