#include "muxer_split.h"

#include "log_helper.h"
#include "error_define.h"

#include <chrono> //chrono
using namespace std::chrono_literals;

namespace am {

	static void process_packet(AVPacket *pkt, AVStream *in_stream,
		AVStream *out_stream)
	{
		pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base,
			out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base,
			out_stream->time_base,
			(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt->duration = (int)av_rescale_q(pkt->duration, in_stream->time_base,
			out_stream->time_base);
		pkt->pos = -1;
	}

	muxer_split::muxer_split()
	{
		al_debug(__FUNCTION__);
	}

	muxer_split::~muxer_split()
	{
		al_debug(__FUNCTION__);
		write_trailer();
	}

	void muxer_split::start_split_record(AVFormatContext* ctx_src)
	{
		al_debug(__FUNCTION__);
		
		_split_running = true;
		_fmt_ctx_src = ctx_src;

		//gene split index correspond AVFormatContext
		//_split_index++;
		//init_split_cxt(_fmt_ctx_src);

		_split_thread = std::thread([=]() {
			while (_split_running) {
				//gene split index correspond AVFormatContext

				_split_from_pts += _split_index * _split_duration.load() * 1000;
				_split_end_pts = _split_from_pts + _split_duration.load() * 1000;
				_split_index++;
				init_split_cxt(_fmt_ctx_src);

				//av_seek_frame(_fmt_ctx_split, -1, _split_from_pts * AV_TIME_BASE, AVSEEK_FLAG_ANY);

				//wait for split_duration, stop current split record.
				std::unique_lock<std::mutex> lk(_split_cv_mutex);
				if (_split_cv.wait_for(lk, _split_duration.load() * 1000ms, [=] {return !_split_running.load(); })) {
					al_debug("no timeout.");
					break;
				} else {
					al_debug("timeout.");
					write_trailer();
				}
				//loop
			}
		});
	}

	void muxer_split::stop_split_record()
	{
		al_debug(__FUNCTION__);

		write_trailer();

		{
			std::lock_guard<std::mutex> autolock(_split_cv_mutex);
			_split_running = false;
		}
		_split_cv.notify_all();

		if (_split_thread.joinable())
			_split_thread.join();
	}

	int muxer_split::write_packet(AVPacket *pkt)
	{
		int ret, error = AE_NO;

		std::lock_guard<std::mutex> autolock(_split_mutex);
		//al_debug("ctx_format: %p\n", _fmt_ctx_split);
		if (_fmt_ctx_split) {
			process_packet(pkt, _fmt_ctx_src->streams[pkt->stream_index],
				_fmt_ctx_split->streams[pkt->stream_index]);
			ret = av_interleaved_write_frame(_fmt_ctx_split, pkt);
			av_packet_unref(pkt);

			if (ret < 0 && ret != -22) {
				error = AE_FFMPEG_WRITE_FRAME_FAILED;
			}
		}

		return error;
	}

	int muxer_split::init_split_cxt(AVFormatContext* ctx_src)
	{
		al_debug("init_split_cxt...");

		int error = AE_NO;
		int ret = 0;

		_fmt_ctx_src = ctx_src;

		char szbuf[256] = "";
		sprintf_s(szbuf, "save-back-%d.flv", _split_index.load());
		_split_path = szbuf;

		al_debug(__FUNCTION__);

		do {
			error = open_dst(&_fmt_ctx_split, _split_path.c_str(), _fmt_ctx_src);
			if (error != AE_NO) {
				break;
			}

			int ret = avformat_write_header(_fmt_ctx_split, NULL);
			if (ret < 0) {
				error = AE_FFMPEG_WRITE_HEADER_FAILED;
				break;
			}
		} while (0);

		return error;
	}

	int muxer_split::write_trailer()
	{
		al_debug("write split file trailer...");

		std::lock_guard<std::mutex> autolock(_split_mutex);
		if (_fmt_ctx_split && !(_fmt_ctx_split->oformat->flags & AVFMT_NOFILE))
			avio_close(_fmt_ctx_split->pb);

		avformat_free_context(_fmt_ctx_split);
		_fmt_ctx_split = nullptr;

		return 0;
	}

	int muxer_split::open_src(AVFormatContext **ctx, const char *path)
	{
		int ret = avformat_open_input(ctx, path, NULL, NULL);
		if (ret < 0) {
			return AE_FFMPEG_OPEN_INPUT_FAILED;
		}

		ret = avformat_find_stream_info(*ctx, NULL);
		if (ret < 0) {
			return AE_FFMPEG_FIND_STREAM_FAILED;
		}

		av_dump_format(*ctx, 0, path, false);

		return AE_NO;
	}
	int muxer_split::open_dst(AVFormatContext **ctx_dst, const char *path, AVFormatContext *ctx_src)
	{
		int ret = 0;

		std::lock_guard<std::mutex> autolock(_split_mutex);
		avformat_alloc_output_context2(ctx_dst, NULL, NULL,
			path);
		if (!*ctx_dst) {
			return AE_FFMPEG_ALLOC_CONTEXT_FAILED;
		}

		for (unsigned i = 0; i < ctx_src->nb_streams; i++) {
			AVStream *in_stream = ctx_src->streams[i];
			AVStream *out_stream = avformat_new_stream(
				*ctx_dst, in_stream->codec->codec);
			if (!out_stream) {
				return AE_FFMPEG_NEW_STREAM_FAILED;
			}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
			AVCodecParameters *par = avcodec_parameters_alloc();
			ret = avcodec_parameters_from_context(par, in_stream->codec);
			if (ret == 0)
				ret = avcodec_parameters_to_context(out_stream->codec,
					par);
			avcodec_parameters_free(&par);
#else
			ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
#endif

			if (ret < 0) {
				return AE_FFMPEG_COPY_PARAMS_FAILED;
			}
			out_stream->time_base = out_stream->codec->time_base;

			av_dict_copy(&out_stream->metadata, in_stream->metadata, 0);

			out_stream->codec->codec_tag = 0;
			if ((*ctx_dst)->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

		av_dump_format(*ctx_dst, 0, path, true);

		if (!((*ctx_dst)->oformat->flags & AVFMT_NOFILE)) {
			ret = avio_open(&(*ctx_dst)->pb, path,
				AVIO_FLAG_WRITE);
			if (ret < 0) {
				return AE_FFMPEG_OPEN_IO_FAILED;
			}
		}

		return AE_NO;
	}

}// namespace am