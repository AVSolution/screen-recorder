#ifndef MUXEX_SPLIT
#define MUXEX_SPLIT

#include <thread>
#include <list>
#include <functional>
#include <math.h>
#include <mutex>
#include <atomic>

#include "headers_ffmpeg.h"

namespace am{

	class muxer_split
	{
	public:
		muxer_split();
		virtual ~muxer_split();

		void start_split_record(AVFormatContext* ctx_src);
		void stop_split_record();

		int write_packet(AVPacket *pkt);

	protected:
		int init_split_cxt(AVFormatContext* ctx_src);
		int write_trailer();

		int open_src(AVFormatContext **ctx, const char *path);
		int open_dst(AVFormatContext **ctx_dst, const char *path, AVFormatContext *ctx_src);

protected:
		AVFormatContext *_fmt_ctx_src = nullptr;
		AVFormatContext *_fmt_ctx_split = nullptr;

		std::atomic<int> _split_duration;
		std::atomic<int> _split_index = 0;
		std::string _split_path;
		std::mutex _split_mutex;
		double _split_from_pts = 0.0;
		double _split_end_pts = 0.0;

		std::thread _split_thread;
		std::atomic<bool> _split_running = false;
		std::condition_variable _split_cv;
		std::mutex _split_cv_mutex;
	};
}//namespace am

#endif

