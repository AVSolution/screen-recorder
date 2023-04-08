#pragma once

#include <functional>
#include <atomic>

namespace am {

	class record_audio;
	class record_desktop;

	struct MUX_STREAM_T;
	struct MUX_SETTING_T;

	typedef std::function<void(const uint8_t *data, int size, int width, int height,int type)> cb_yuv_data;
	typedef std::function<void(const char* path, int index, int over, int error)> cb_split_progress;

	class muxer_file
	{
	public:
		muxer_file();
		virtual ~muxer_file();

		virtual int init(
			const char *output_file,
			record_desktop *source_desktop,
			record_audio ** source_audios,
			const int source_audios_nb,
			const MUX_SETTING_T &setting
		) = 0;

		virtual int start() = 0;
		virtual int stop() = 0;

		virtual int pause() = 0;
		virtual int resume() = 0;

		inline void register_yuv_data(cb_yuv_data on_yuv_data) {
			_on_yuv_data = on_yuv_data;
		}

		inline void set_preview_enabled(bool enable) {
			_preview_enabled = enable;
		}

		inline void register_split_progress(cb_split_progress on_split_progress) {
			_on_split_progress = on_split_progress;
		}

	protected:
		cb_yuv_data _on_yuv_data;

		std::atomic_bool _inited;
		std::atomic_bool _running;
		std::atomic_bool _paused;
		std::atomic_bool _preview_enabled;

		bool _have_v, _have_a;

		std::string _output_file;

		bool is_split;
		cb_split_progress _on_split_progress;
	};


}