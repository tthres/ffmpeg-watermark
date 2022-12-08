#pragma execution_character_set("utf-8")
#include <string>
#include <iostream>
#include <thread>
#include <memory>
#include <fstream>
#include <time.h>
#include "watermark.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
}
using namespace std;
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"avdevice")
#pragma comment(lib,"avfilter")
#pragma comment(lib,"freetype.lib")

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//输入媒体文件的上下文
AVFormatContext* input_format_ctx = nullptr;

//输出媒体文件的上下文
AVFormatContext* output_format_ctx;

//输出视频编码器
AVCodecContext* ouput_video_encode_ctx = NULL;

//音视频解码器
AVCodecContext* video_decode_ctx = NULL;
AVCodecContext* audio_decode_ctx = NULL;

//视频索引和音频索引
int video_stream_index = -1;
int audio_stream_index = -1;


//输出编码器
static AVCodec* output_video_codec;

//滤镜容器和缓存
AVFilterGraph* filter_graph = nullptr;
AVFilterContext* buffersink_ctx = nullptr;;
AVFilterContext* buffersrc_ctx = nullptr;;
AVPacket packet;

//起始时间
static int64_t startTime;

//变量定义   
BITMAPFILEHEADER strHead;
BITMAPINFOHEADER strInfo;
IMAGEDATA* arrayColor;
int bmpWidth, bmpHeight;

int OpenOutput(char* fileName)
{
	//创建输出流,输出mp4格式视频
	int ret = 0;
	ret = avformat_alloc_output_context2(&output_format_ctx, NULL, "mp4", fileName);
	if (ret < 0)
	{
		return -1;
	}

	//打开输出流
	ret = avio_open(&output_format_ctx->pb, fileName, AVIO_FLAG_READ_WRITE);
	if (ret < 0)
	{
		return -2;
	}

	//创建输出流
	for (int index = 0; index < input_format_ctx->nb_streams; index++)
	{
		if (index == video_stream_index)
		{
			AVStream* stream = avformat_new_stream(output_format_ctx, output_video_codec);
			avcodec_parameters_from_context(stream->codecpar, ouput_video_encode_ctx);
			stream->codecpar->codec_tag = 0;
		}
		else if (index == audio_stream_index)
		{
			AVStream* stream = avformat_new_stream(output_format_ctx, NULL);
			stream->codecpar = input_format_ctx->streams[audio_stream_index]->codecpar;
			stream->codecpar->codec_tag = 0;
		}
	}
	//写文件头
	ret = avformat_write_header(output_format_ctx, nullptr);
	if (ret < 0)
	{
		return -3;
	}
	if (ret >= 0)
		cout << "open output stream successfully" << endl;
	return ret;
}

//初始化输出视频的编码器
int InitEncoderCodec(int iWidth, int iHeight)
{
	output_video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (NULL == output_video_codec)
	{
		return  -1;
	}
	//指定编码器的参数
	ouput_video_encode_ctx = avcodec_alloc_context3(output_video_codec);
	ouput_video_encode_ctx->time_base = input_format_ctx->streams[video_stream_index]->time_base;
	ouput_video_encode_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ouput_video_encode_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	ouput_video_encode_ctx->width = iWidth;
	ouput_video_encode_ctx->height = iHeight;
	ouput_video_encode_ctx->bit_rate = input_format_ctx->streams[video_stream_index]->codecpar->bit_rate;
	ouput_video_encode_ctx->pix_fmt = (AVPixelFormat)*output_video_codec->pix_fmts;
	ouput_video_encode_ctx->profile = FF_PROFILE_H264_MAIN;
	ouput_video_encode_ctx->level = 41;
	ouput_video_encode_ctx->thread_count = 8;
	ouput_video_encode_ctx->has_b_frames = 0;
	ouput_video_encode_ctx->max_b_frames = 0;
	return 0;
}


//将加水印之后的图像帧输出到文件中
static int output_frame(AVFrame* frame, AVRational time_base)
{
	int code;
	AVPacket packet = { 0 };
	av_init_packet(&packet);

	int ret = avcodec_send_frame(ouput_video_encode_ctx, frame);
	if (ret < 0)
	{
		printf("Error sending a frame for encoding\n");
		return -1;
	}
	while (ret >= 0)
	{
		ret = avcodec_receive_packet(ouput_video_encode_ctx, &packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return (ret == AVERROR(EAGAIN)) ? 0 : 1;
		}
		else if (ret < 0) {
			printf("Error during encoding\n");
			exit(1);
		}
#if 0
		AVRational avTimeBaseQ = { 1, AV_TIME_BASE };
		int64_t ptsTime = av_rescale_q(frame->pts, input_format_ctx->streams[video_stream_index]->time_base, avTimeBaseQ);
		int64_t nowTime = av_gettime() - startTime;

		if ((ptsTime > nowTime))
		{
			int64_t sleepTime = ptsTime - nowTime;
			av_usleep((sleepTime));
		}
		else
		{
			printf("not sleeping\n");
		}
#endif
		packet.pts = av_rescale_q_rnd(packet.pts, time_base, output_format_ctx->streams[video_stream_index]->time_base, (AVRounding)(AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
		packet.dts = av_rescale_q_rnd(packet.dts, time_base, output_format_ctx->streams[video_stream_index]->time_base, (AVRounding)(AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
		packet.stream_index = video_stream_index;
		code = av_interleaved_write_frame(output_format_ctx, &packet);
		av_packet_unref(&packet);

		if (code < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "[ERROR] Writing Live Stream Interleaved Frame");
		}

		if (ret < 0) {
			exit(1);
		}
		av_packet_unref(&packet);
	}
}

int readBmp(FILE *fpi) {
	fread(&strHead, 1, sizeof(tagBITMAPFILEHEADER), fpi);

	if (0x4d42 != strHead.bfType) {
		av_log(NULL, AV_LOG_ERROR, "the file is not a bmp file!\n");
		return -1;
	}
	fread(&strInfo, 1, sizeof(tagBITMAPINFOHEADER), fpi);
	bmpWidth = strInfo.biWidth;
	bmpHeight = strInfo.biHeight;
	arrayColor = new IMAGEDATA[bmpWidth * bmpHeight];
	fread(arrayColor, 1, bmpWidth * bmpHeight * sizeof(IMAGEDATA), fpi);
	long long N = bmpWidth * bmpHeight;
	for (long long i = 0; i < N / 2; i++) {
		swap(arrayColor[i], arrayColor[N - 1 - i]);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	// _CrtSetBreakAlloc(461);
	
	// 初始化水印
	watermark myMark;
	// 打开添加水印的图片
	FILE* fpi = fopen("mv_logo.bmp", "rb");
	if (!fpi) cout << "open bmp file failed !!!" << endl;
	else if (readBmp(fpi) == -1) myMark.flag = 2;
	

	clock_t start_time, end_time;
	start_time = clock();
	//输入文件地址、输出文件地址
	string fileInput = std::string("new.mp4");
	string fileOutput = std::string("out/output.mp4");

	//初始化各种配置
	avformat_network_init();
	av_log_set_level(AV_LOG_ERROR);

	//打开输入文件
	int ret = avformat_open_input(&input_format_ctx, fileInput.c_str(), NULL, NULL);
	if (ret < 0)
	{
		return  ret;
	}
	av_log(NULL, AV_LOG_INFO, "open input file done\n");
	ret = avformat_find_stream_info(input_format_ctx, NULL);
	if (ret < 0)
	{
		return ret;
	}

	//查找音视频流的索引
	for (int index = 0; index < input_format_ctx->nb_streams; ++index)
	{
		if (index == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = index;
		}
		else if (index == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = index;
		}
	}

	//打开视频解码器
	const AVCodec* codec = avcodec_find_decoder(input_format_ctx->streams[video_stream_index]->codecpar->codec_id);
	if (!codec)
	{
		return -1;
	}
	video_decode_ctx = avcodec_alloc_context3(codec);
	if (!video_decode_ctx)
	{
		fprintf(stderr, "Could not allocate video codec context\n");
		return -2;
	}
	avcodec_parameters_to_context(video_decode_ctx, input_format_ctx->streams[video_stream_index]->codecpar);
	if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
		video_decode_ctx->flags |= AV_CODEC_FLAG_TRUNCATED;

	ret = avcodec_open2(video_decode_ctx, codec, NULL);
	if (ret < 0)
	{
		av_free(video_decode_ctx);
		return -3;
	}

	//初始化视频编码器
	ret = InitEncoderCodec(video_decode_ctx->width, video_decode_ctx->height);
	if (ret < 0)
	{
		return 0;
	}
	cout << "init encoder done" << endl;

	//打开编码器
	ret = avcodec_open2(ouput_video_encode_ctx, output_video_codec, NULL);
	if (ret < 0)
	{
		return  ret;
	}

	//初始化输出
	if (OpenOutput((char*)fileOutput.c_str()) < 0)
	{
		cout << "Open file Output failed!" << endl;
		this_thread::sleep_for(chrono::seconds(10));
		return 0;
	}

	
	AVFrame* filterFrame = av_frame_alloc();

	av_init_packet(&packet);
	startTime = av_gettime();
	cout << "decoding..." << endl;
	cout << "muxing..." << endl;
	while (true)
	{
		int ret = av_read_frame(input_format_ctx, &packet);
		if (ret < 0)
		{
			break;
		}
		//视频帧通过滤镜处理之后编码输出
		if (packet.stream_index == video_stream_index)
		{
			int ret = avcodec_send_packet(video_decode_ctx, &packet);
			if (ret < 0)
			{
				break;
			}
			
			while (ret >= 0)
			{
				AVFrame* pSrcFrame = av_frame_alloc();
				ret = avcodec_receive_frame(video_decode_ctx, pSrcFrame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				{
					break;
				}
				else if (ret < 0)
				{
					goto End;
				}
				
				
				// 添加水印
				if (myMark.flag == 1) {		// 图片
					myMark.BmpinsertToBmp(pSrcFrame, filterFrame, arrayColor, bmpWidth, bmpHeight);
				}
				else if (myMark.flag == 2)  // 文字
					myMark.WordInsertToBmp(pSrcFrame, filterFrame);
				
				//编码之后输出
				output_frame(filterFrame, input_format_ctx->streams[video_stream_index]->time_base);
				av_frame_unref(filterFrame);
				
				av_frame_unref(pSrcFrame);
				
			}
			
		}
		else if (packet.stream_index == audio_stream_index)
		{
			packet.pts = av_rescale_q_rnd(packet.pts, input_format_ctx->streams[audio_stream_index]->time_base, output_format_ctx->streams[audio_stream_index]->time_base, (AVRounding)(AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
			packet.dts = av_rescale_q_rnd(packet.dts, input_format_ctx->streams[audio_stream_index]->time_base, output_format_ctx->streams[audio_stream_index]->time_base, (AVRounding)(AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
			packet.stream_index = audio_stream_index;
			av_interleaved_write_frame(output_format_ctx, &packet);
		}
		av_packet_unref(&packet);
	}
	av_write_trailer(output_format_ctx);

End:
	//结束的时候清理资源
	avfilter_graph_free(&filter_graph);
	if (input_format_ctx != NULL)
	{
		avformat_close_input(&input_format_ctx);
	}
	avcodec_free_context(&video_decode_ctx);
	avcodec_free_context(&ouput_video_encode_ctx);
	fclose(fpi);
	cout << "mux done" << endl;
	end_time = clock();
	cout << "time = " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;
	

	//_CrtDumpMemoryLeaks();
	return 0;
}