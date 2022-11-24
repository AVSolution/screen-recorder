#pragma once

#include <atomic>

#include "headers_ffmpeg.h"

namespace am {

	class sws_helper
	{
	public:
		sws_helper();
		~sws_helper();
		
		int init(
			AVPixelFormat src_fmt,int src_width,int src_height,
			AVPixelFormat dst_fmt,int dst_width,int dst_height
		);

		int convert(const AVFrame *frame, uint8_t ** out_data, int *len);

	private:
		void cleanup();

	private:
		std::atomic_bool _inited;

		AVFrame *_frame;

		uint8_t *_buffer;
		int _buffer_size;

		struct SwsContext *_ctx;

		AVPixelFormat _src_fmt{ AV_PIX_FMT_NONE };
		std::atomic_int _src_width{ 0 };
		std::atomic_int _src_height{ 0 };
		AVPixelFormat _dst_fmt{ AV_PIX_FMT_NONE };
		std::atomic_int _dst_width{ 0 };
		std::atomic_int _dst_height{ 0 };
	};

}