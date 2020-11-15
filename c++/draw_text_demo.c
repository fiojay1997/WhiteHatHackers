bool CPenWordIntoPic::SetSubTitile(const char* subTitile, AVCodecContext * codecContext)
{
	m_codecContext = codecContext;
	if (!codecContext)
	{
		return false;
	}
 
	m_ConverImg = new CConvertImg(codecContext->width, 
									codecContext->height, 
									codecContext->width, 
									codecContext->height, 
									codecContext->pix_fmt, 
									codecContext->pix_fmt);
 
	std::string tmpStr = subTitile;
	
	char tmpChar[128] = {0};
	sprintf(tmpChar, "fontsize=%d:x=0:y=0:text=", 100);
	std::string strFontAndPos = tmpChar;
 
 
	m_filters_descr ="drawtext=fontfile=msyh.ttf:fontcolor=red:" + strFontAndPos + tmpStr;
	
	if(0 != InitFilter(codecContext))
		return false;
 
	return true;
}

int CPenWordIntoPic::InitFilter(AVCodecContext * codecContext)
{
	char args[512];
	int ret = 0;   
 
	AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("buffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P};
 
	m_filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !m_filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
 
	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	sprintf(args,
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		codecContext->width, codecContext->height, codecContext->pix_fmt,
		codecContext->time_base.num, codecContext->time_base.den,
		codecContext->sample_aspect_ratio.num, codecContext->sample_aspect_ratio.den);
 
	ret = avfilter_graph_create_filter(&m_buffersrc_ctx, buffersrc, "in",
		args, NULL, m_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}
 
	/* buffer video sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&m_buffersink_ctx, buffersink, "out",
		NULL, NULL, m_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}
 
	ret = av_opt_set_int_list(m_buffersink_ctx, "pix_fmts", pix_fmts,
		AV_PIX_FMT_YUV420P, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}
 
	/* Endpoints for the filter graph. */
	outputs->name       = av_strdup("in");
	outputs->filter_ctx = m_buffersrc_ctx;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;
 
	inputs->name       = av_strdup("out");
	inputs->filter_ctx = m_buffersink_ctx;
	inputs->pad_idx    = 0;
	inputs->next       = NULL;   
	if ((ret = avfilter_graph_parse_ptr(m_filter_graph, m_filters_descr.c_str(),
		&inputs, &outputs, NULL)) < 0)
		goto end;
 
	if ((ret = avfilter_graph_config(m_filter_graph, NULL)) < 0)
		goto end;
	return ret;
end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}