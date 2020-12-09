#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>
#include <cairo.h>

extern "C"
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libavutil/opt.h>
}

using namespace std;

class video
{
private:
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

public:
    video(const string & filename_, const unsigned int width_, const unsigned int height_) 
            : width(width_), height(height_), iframe(0), pixels(4 * width * height)
    {
        cairo_surface = cairo_image_surface_create_for_data(
		    (unsigned char*) &pixels[0], CAIRO_FORMAT_RGB24, width, height,
		    cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width));
        
        if (cairo_surface == NULL)
        {
            cout << "load cario_surface failed" << endl;
            exit(1);
        }

        swsCtx = sws_getContext(width, height,
            AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUVJ420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        
        if (swsCtx == NULL)
        {
            cout << "load sws context failed" << endl;
            exit(1);
        }

        const char* fmtext = "mp4";
        const string filename = filename_ + "." + fmtext;
        fmt = av_guess_format(fmtext, NULL, NULL);

        if (fmt == NULL)
        {
            cout << "load format failed" << endl;
            exit(1);
        }

        avformat_alloc_output_context2(&fc, NULL, NULL, filename.c_str());
        
        if (fc == NULL)
        {
            cout << "load format conetxt failed" << endl;
            exit(1);
        }

        AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);

        if (codec == NULL)
        {
            cout << "load encoder failed" << endl;
            exit(1);
        }

        AVDictionary* opt = NULL;
        av_dict_set(&opt, "preset", "slow", 0);
        av_dict_set(&opt, "crf", "20", 0);
        stream = avformat_new_stream(fc, codec);

        if (stream == NULL)
        {
            cout << "load stream failed" << endl;
            exit(1);
        }

        c = stream->codec;

        if (c == NULL)
        {
            cout << "load stream encoder failed" << endl;
            exit(1);
        }
        c->width = width;
        c->height = height;
        c->pix_fmt = AV_PIX_FMT_YUVJ420P;
        c->time_base = (AVRational){ 1, 25 };

        if (fc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        
        int error_code;
        error_code = avcodec_open2(c, codec, &opt);
        if (error_code != 0)
        {
            cout << "avcodec_open failed" << endl;
            exit(1);
        }

       //  av_dict_free(&opt);
        
        stream->time_base = (AVRational){ 1, 25 };
       
        error_code = avio_open(&fc->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (error_code != 0)
        {
            cout << "avio_open failed" << endl;
            exit(1);
        }

        int ret = avformat_write_header(fc, &opt);

        if (ret != 0)
        {
            cout << "avformat write header failed" << endl;
            exit(1);
        }



        rgbpic = av_frame_alloc();

        if (rgbpic == NULL)
        {
            cout << "load rgb frame failed" << endl;
            exit(1);
        }

        rgbpic->format = AV_PIX_FMT_RGB24;
        rgbpic->width = width;
        rgbpic->height = height;
        ret = av_frame_get_buffer(rgbpic, 1);

        yuvpic = av_frame_alloc();

        if (yuvpic == NULL)
        {
            cout << "load yuv frame failed" << endl;
            exit(1);
        }

        yuvpic->format = AV_PIX_FMT_YUVJ420P;
        yuvpic->width = width;
        yuvpic->height = height;
        ret = av_frame_get_buffer(yuvpic, 1);
    }

    void addFrame(const string & filename)
    {
        cairo_surface_t* img = cairo_image_surface_create_from_png(filename.c_str());

		int imgw = cairo_image_surface_get_width(img);
		int imgh = cairo_image_surface_get_height(img);

		cairo_t* cr = cairo_create(cairo_surface);
		cairo_scale(cr, (float)width / imgw, (float)height / imgh);
		cairo_set_source_surface(cr, img, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(img);

		unsigned char* data = cairo_image_surface_get_data(cairo_surface);
		
		memcpy(&pixels[0], data, pixels.size());

        addFrame((uint8_t*)&pixels[0]);
    }

        
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
