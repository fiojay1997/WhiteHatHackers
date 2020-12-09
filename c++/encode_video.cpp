/**
 * TEAM: WHITEHATHACKERS 
 * MEMBERS: Jake, Leon, Nanda, Kaijie, Sephora, Austin
 * DATE: 10/08/2020
 * 
 **/

#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>
#include <cairo.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

//include c files
extern "C"
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libavutil/opt.h>
}

using namespace std;

/**
 * Class: video 
 * Object that represends the timeline video
 **/
class video
{
private:

    //Allocate variables needed for ffmpeg/pixel manipulation and logging
    const unsigned int width, height;
	unsigned int iframe;

	SwsContext* swsCtx;
	AVOutputFormat* fmt;
	AVStream* stream;
	AVFormatContext* fc;
	AVCodecContext* c;
	AVPacket pkt;

	AVFrame *rgbpic, *yuvpic;

	vector<uint8_t> pixels;

	cairo_surface_t* cairo_surface;

    std::shared_ptr<spdlog::logger>  mylogger = spdlog::basic_logger_mt("mylogger", "encode_video_logs.txt");

public:

    /**
     * Video Constructor
     * Prepares the video for adding frames
     * Initialize FFMPEG Contexts/Formats/Codecs/Frame/etc.
     * Initialize Cario Surface
     **/
    video(const string & filename_, const unsigned int width_, const unsigned int height_) 
            : width(width_), height(height_), iframe(0), pixels(4 * width * height)
    {
        cairo_surface = cairo_image_surface_create_for_data(
		    (unsigned char*) &pixels[0], CAIRO_FORMAT_RGB24, width, height,
		    cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width));
        
        if (cairo_surface == NULL)
        {
	        mylogger->error("load cario_surface failed");
            //cout << "load cario_surface failed" << endl;
            exit(1);
        }

        //create sws context for formatting the video
        swsCtx = sws_getContext(width, height,
            AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUVJ420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        
        if (swsCtx == NULL)
        {
	        mylogger->error("load sws context failed");
            //cout << "load sws context failed" << endl;
            exit(1);
        }

        const char* fmtext = "mp4";
        const string filename = filename_ + "." + fmtext;

        //guess output format based on filename
        fmt = av_guess_format(fmtext, NULL, NULL);

        if (fmt == NULL)
        {
	        mylogger->error("load format failed");
            //cout << "load format failed" << endl;
            exit(1);
        }

        //allocate output context for video
        avformat_alloc_output_context2(&fc, NULL, NULL, filename.c_str());
        
        if (fc == NULL)
        {
	        mylogger->error("load format context failed");
            //cout << "load format conetxt failed" << endl;
            exit(1);
        }

        //initialize codec
        AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);

        if (codec == NULL)
        {
	        mylogger->error("load encoder failed");
            //cout << "load encoder failed" << endl;
            exit(1);
        }

        //set options for video
        AVDictionary* opt = NULL;
        av_dict_set(&opt, "preset", "slow", 0);
        av_dict_set(&opt, "crf", "20", 0);

        //create a video stream
        stream = avformat_new_stream(fc, codec);

        if (stream == NULL)
        {
	        mylogger->error("load stream failed");
            //cout << "load stream failed" << endl;
            exit(1);
        }

        //initialize codec context based on the stream
        c = stream->codec;

        if (c == NULL)
        {
	        mylogger->error("load stream encoder failed");
            //cout << "load stream encoder failed" << endl;
            exit(1);
        }

        //set the out context parameters
        c->width = width;
        c->height = height;
        c->pix_fmt = AV_PIX_FMT_YUVJ420P;
        c->time_base = (AVRational){ 1, 25 };

        if (fc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        
        int error_code;

        //open the codec
        error_code = avcodec_open2(c, codec, &opt);
        if (error_code != 0)
        {
	        mylogger->error("avcodec_open failed");
            //cout << "avcodec_open failed" << endl;
            exit(1);
        }

       //  av_dict_free(&opt);
        
        //set fps
        stream->time_base = (AVRational){ 1, 25 };
       
        //open file with output context
        error_code = avio_open(&fc->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (error_code != 0)
        {
	        mylogger->error("avio_open failed");
            //cout << "avio_open failed" << endl;
            exit(1);
        }

        //write header
        int ret = avformat_write_header(fc, &opt);

        if (ret != 0)
        {
	        mylogger->error("avformat write header failed");
            //cout << "avformat write header failed" << endl;
            exit(1);
        }

        //allocate rgb frame 
        rgbpic = av_frame_alloc();

        if (rgbpic == NULL)
        {
	        mylogger->error("load rgb frame failed");
            //cout << "load rgb frame failed" << endl;
            exit(1);
        }

        rgbpic->format = AV_PIX_FMT_RGB24;
        rgbpic->width = width;
        rgbpic->height = height;
        ret = av_frame_get_buffer(rgbpic, 1);

        //allocate yuv frame
        yuvpic = av_frame_alloc();

        if (yuvpic == NULL)
        {
	    mylogger->error("load yuv frame failed");
            ///cout << "load yuv frame failed" << endl;
            exit(1);
        }
        yuvpic->format = AV_PIX_FMT_YUVJ420P;
        yuvpic->width = width;
        yuvpic->height = height;

        //allocate buffer
        ret = av_frame_get_buffer(yuvpic, 1);
    }

    /**
     * Creates an image with Cairo 
     * Parses image into pixel data
     * Adds frame to video
     * 
     **/
    void addFrame(const string & filename)
    {
        //create cairo image with file
        cairo_surface_t* img = cairo_image_surface_create_from_png(filename.c_str());

        // get width / height
		int imgw = cairo_image_surface_get_width(img);
		int imgh = cairo_image_surface_get_height(img);

        //create surface and paint
		cairo_t* cr = cairo_create(cairo_surface);
		cairo_scale(cr, (float)width / imgw, (float)height / imgh);
		cairo_set_source_surface(cr, img, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(img);

        //gather pixels from cario surface
		unsigned char* data = cairo_image_surface_get_data(cairo_surface);
		
        //copy pixels to this object
		memcpy(&pixels[0], data, pixels.size());

        addFrame((uint8_t*)&pixels[0]);
    }

    /**
     * Helper function for addFrame(const string & filename)
     * Pixel manipulation in the rgb frame, set params, encode into video
     **/
    void addFrame(const uint8_t* pixels)
    {

        for (unsigned int y = 0; y < height; y++)
        {
            for (unsigned int x = 0; x < width; x++)
            {
                rgbpic->data[0][y * rgbpic->linesize[0] + 3 * x + 0] = pixels[y * 4 * width + 4 * x + 2];
                rgbpic->data[0][y * rgbpic->linesize[0] + 3 * x + 1] = pixels[y * 4 * width + 4 * x + 1];
                rgbpic->data[0][y * rgbpic->linesize[0] + 3 * x + 2] = pixels[y * 4 * width + 4 * x + 0];
            }
        }


        sws_scale(swsCtx, rgbpic->data, rgbpic->linesize, 0,
            height, yuvpic->data, yuvpic->linesize);

        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        yuvpic->pts = iframe;

        int got_output;
        int ret = avcodec_encode_video2(c, &pkt, yuvpic, &got_output);
        if (got_output)
        {
            fflush(stdout);
            av_packet_rescale_ts(&pkt, (AVRational){ 1, 25 }, stream->time_base);

            pkt.stream_index = stream->index;
            printf("Writing frame %d (size = %d)\n", iframe++, pkt.size);

            av_interleaved_write_frame(fc, &pkt);
            av_packet_unref(&pkt);
        }
    }

    /**
     * Video Destructor
     * Write delayed frames
     * Deallocate Memory
     **/
    ~video()
    {
        for (int got_output = 1; got_output; )
        {
            int ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
            if (got_output)
            {
                fflush(stdout);
                av_packet_rescale_ts(&pkt, (AVRational){ 1, 25 }, stream->time_base);
                pkt.stream_index = stream->index;
                av_interleaved_write_frame(fc, &pkt);
                av_packet_unref(&pkt);
            }
        }
        
        av_write_trailer(fc);

        if (!(fmt->flags & AVFMT_NOFILE))
            avio_closep(&fc->pb);
        avcodec_close(stream->codec);

        sws_freeContext(swsCtx);
        av_frame_free(&rgbpic);
        av_frame_free(&yuvpic);
        avformat_free_context(fc);

        cairo_surface_destroy(cairo_surface);
    }
};
