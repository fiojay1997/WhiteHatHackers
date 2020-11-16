#include <stdlib.h>
#include <stdio.h>
// formatter
#include "libavformat/avformat.h"
// decoder
#include "libavcodec/avcodec.h"
// sizing
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
// filter
#include "libavfilter/avfilter.h"
#include "libavutil/log.h"
#include <libavutil/opt.h>
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"


AVFormatContext * context = NULL;
AVFormatContext* outputContext;
int64_t  lastPts = 0;
int64_t  lastDts = 0;
int64_t lastFrameRealtime = 0;

int64_t firstPts = AV_NOPTS_VALUE;
int64_t startTime = 0;

AVCodecContext*	outPutEncContext = NULL;
AVCodecContext *decoderContext = NULL;
#define SrcWidth 1920
#define SrcHeight 1080
#define DstWidth 640
#define DstHeight 480

struct SwsContext* pSwsContext;

AVFilterGraph * filter_graph = NULL;
AVFilterContext *buffersink_ctx  = NULL;;
AVFilterContext *buffersrc_ctx  = NULL;;


int interrupt_cb(void *ctx)
{
	return 0;
}

void Init()
{
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_ERROR);
}

int open_input_context(char *fileName)
{
	context = avformat_alloc_context();
	context->interrupt_callback.callback = interrupt_cb;
	AVDictionary *format_opts =  NULL;

	int ret = avformat_open_input(&context, fileName, NULL, &format_opts);
	if(ret < 0)
	{
		return  ret;
	}
	ret = avformat_find_stream_info(context,NULL);
	av_dump_format(context, 0, fileName, 0);
	if(ret >= 0) 
	{
		printf("open input stream successfully \n");
	}
	return ret;
}

AVPacket* ReadPacketFromSource()
{
	AVPacket* packet = av_packet_alloc();
	int ret = av_read_frame(context, packet);
	if(ret >= 0)
	{
		return packet;
	}
	else
	{
		return NULL;
	}
}


int open_output_context(char *fileName)
{
	int ret = 0;
	ret  = avformat_alloc_output_context2(&outputContext, NULL, "mpegts", fileName);
	if(ret < 0)
	{
		goto Error;
	}
	ret = avio_open2(&outputContext->pb, fileName, AVIO_FLAG_READ_WRITE,NULL, NULL);	
	if(ret < 0)
	{
		goto Error;
	}

	for(int i = 0; i < context->nb_streams; i++)
	{
		AVStream * stream = avformat_new_stream(outputContext, outPutEncContext->codec);
		stream->codec = outPutEncContext;
		if(ret < 0)
		{
			goto Error;
		}
	}
	av_dump_format(outputContext, 0, fileName, 1);
	ret = avformat_write_header(outputContext, NULL);
	if(ret < 0)
	{
		goto Error;
	}
	if(ret >= 0)
		printf("open output stream successfully\n");
	return ret ;
Error:
	if(outputContext)
	{
		avformat_close_input(&outputContext);
	}
	return ret ;
}

void close_input()
{
	if(context != NULL)
	{
		avformat_close_input(&context);
	}
}

void close_output()
{
	if(outputContext != NULL)
	{
		for(int i = 0 ; i < outputContext->nb_streams; i++)
		{
			AVCodecContext *codecContext = outputContext->streams[i]->codec;
			avcodec_close(codecContext);
		}
		avformat_close_input(&outputContext);
	}
}

int init_encode_codec( int iWidth, int iHeight)
{
	AVCodec *  pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if(NULL == pH264Codec)
	{
		printf("%s \n", "avcodec_find_encoder failed");
		return  -1;
	}
	outPutEncContext = avcodec_alloc_context3(pH264Codec);
	outPutEncContext->gop_size = 30;
	outPutEncContext->has_b_frames = 0;
	outPutEncContext->max_b_frames = 0;
	outPutEncContext->codec_id = pH264Codec->id;
	outPutEncContext->time_base.num =context->streams[0]->codec->time_base.num;
	outPutEncContext->time_base.den = context->streams[0]->codec->time_base.den;
	outPutEncContext->pix_fmt            = *pH264Codec->pix_fmts;
	outPutEncContext->width              =  iWidth;
	outPutEncContext->height             = iHeight;

	outPutEncContext->me_subpel_quality = 0;
	outPutEncContext->refs = 1;
	outPutEncContext->scenechange_threshold = 0;
	outPutEncContext->trellis = 0;
	AVDictionary *options = NULL;
	outPutEncContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	int ret = avcodec_open2(outPutEncContext, pH264Codec, &options);
	if (ret < 0)
	{
		printf("%s", "open codec failed");
		return  ret;
	}
	return 1;
}

int init_decode_codec()
{
    decoderContext = context->streams[0]->codec;
	if (!decoderContext) {
		printf( "Could not allocate video codec context\n");
		exit(1);
	}else{
         printf("success \n");
    }
    enum AVCodecID codecId = decoderContext->codec_id;
	AVCodec* codec = avcodec_find_decoder(codecId);
	if(!codec)
	{
		return -1;
	}else{
        printf("success \n");
    }
	
	int ret = avcodec_open2(decoderContext, codec, NULL);
	return ret;

}

int decode_video(AVPacket* packet, AVFrame* frame)
{
	int gotFrame = 0;
	int hr = avcodec_decode_video2(decoderContext, frame, &gotFrame, packet);
	if(hr >= 0 && gotFrame != 0)
	{
		return 1;
	}
	return -1;
}

int init_filter(AVCodecContext * codecContext)
{
	char args[512];
	int ret = 0;	

	AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("buffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	// char*  filters_descr ="drawtext=fontfile=D\\\\:FreeSerif.ttf:fontsize=100:text=hello world:x=100:y=100";
    char*  filters_descr = "drawtext=fontfile=/System/Library/Fonts/Courier.dfont:fontsize=40:text=hello world:x=100:y=100:fontcolor=green";
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P};

	filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		codecContext->width, codecContext->height, codecContext->pix_fmt,
		codecContext->time_base.num, codecContext->time_base.den,
		codecContext->sample_aspect_ratio.num, codecContext->sample_aspect_ratio.den);

	ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
		args, NULL, filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}

	/* buffer video sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
		NULL, NULL, filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}

	ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
		AV_PIX_FMT_YUV420P, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}

	/* Endpoints for the filter graph. */
	outputs->name       = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;

	inputs->name       = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx    = 0;
	inputs->next       = NULL;    
	if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
		&inputs, &outputs, NULL)) < 0)
		goto end;

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;
	return ret;
end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}

int main(int argc, char* argv[])
{	
	// replace this
	char* fileInput = "./jichi_no_audio.ts"; 
	// replace this
	char* fileOutput = "./jichi22.ts";
	Init();
	if(open_input_context(fileInput) < 0)
	{
		printf( "Open file Input failed!\n");

		return 0;
	}

	int ret = init_decode_codec();
	if(ret <0)
	{
		printf( "init_decode_codec failed!\n");
		return 0;
	}else
    {
        printf("init_decode_codec success \n");
    }
    

	ret = init_encode_codec(decoderContext->width,decoderContext->height);
	if(ret < 0)
	{
		printf( "open eccoder failed ret is %d\n",ret);
		printf( "init_encode_codec failed!\n");
		return 0;
	}else{
         printf("init_encode_codec success \n");
    }

	
	if(open_output_context(fileOutput) < 0)
	{
		printf( "Open file Output failed!\n");
		return 0;
	}else{
        printf("open_output_context success \n");
    }

    ret = init_filter(outPutEncContext);
    if(ret < 0){
        printf("init_filter failed \n");
        return 0;
    }else{
         printf("init_filter success \n");
    }

	AVFrame* pSrcFrame = av_frame_alloc();
	AVFrame*  filterFrame = av_frame_alloc();
	int got_output = 0;	
	int64_t  timeRecord = 0;
	int64_t  firstPacketTime = 0;
	int64_t outLastTime = av_gettime();
	int64_t inLastTime = av_gettime();
	int64_t videoCount = 0;
	while(1)
	{
		outLastTime = av_gettime();
		AVPacket* packet = ReadPacketFromSource();
		if(packet)
		{
			if(timeRecord == 0)
			{
				firstPacketTime = av_gettime();
				timeRecord++;
			}
			if(decode_video(packet,pSrcFrame))
			{
				if (av_buffersrc_add_frame_flags(buffersrc_ctx, pSrcFrame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0)
				{
					if (av_buffersink_get_frame(buffersink_ctx, filterFrame) >= 0)
					{
						AVPacket *pTmpPkt = (AVPacket *)av_malloc(sizeof(AVPacket));
						av_init_packet(pTmpPkt);
						pTmpPkt->data = NULL;
						pTmpPkt->size = 0;
						ret = avcodec_encode_video2(outPutEncContext, pTmpPkt, filterFrame, &got_output);
						if (ret >= 0 && got_output)
						{
							int ret = av_write_frame(outputContext, pTmpPkt);
							av_packet_unref(pTmpPkt);
						}
					}
				}
			}
		}
		else break;
	}
    printf("Transcode file end!\n");
    _end:
	close_input();
	close_output();


	return 0;
}
