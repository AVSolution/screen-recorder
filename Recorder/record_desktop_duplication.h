#ifndef RECORD_DESKTOP_DUPLICATION
#define RECORD_DESKTOP_DUPLICATION

#include "record_desktop.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

namespace am {

	class record_desktop_duplication:
		public record_desktop
	{
	public:
		record_desktop_duplication();
		~record_desktop_duplication();

		virtual int init(
			const RECORD_DESKTOP_RECT &rect,
			const int fps);

		virtual int start();
		virtual int pause();
		virtual int resume();
		virtual int stop();

	protected:
		virtual void clean_up();

	private:
		int init_d3d11();

		void clean_d3d11();

		int init_duplication();

		int free_duplicated_frame();

		void clean_duplication();

		bool attatch_desktop();

		int do_record();

		void do_sleep(int64_t dur, int64_t pre, int64_t now);

		void record_func();

	private:
		uint8_t *_buffer;
		uint32_t _buffer_size;
		uint32_t _width, _height;

		HMODULE _d3d11, _dxgi;

		ID3D11Device* _d3d_device;
		ID3D11DeviceContext* _d3d_ctx;
		ID3D11VertexShader* _d3d_vshader;
		ID3D11PixelShader* _d3d_pshader;
		ID3D11InputLayout* _d3d_inlayout;
		ID3D11SamplerState* _d3d_samplerlinear;

		IDXGIOutputDuplication *_duplication;
		ID3D11Texture2D *_image;
		DXGI_OUTPUT_DESC _output_des;
		int _output_index;

	};

}

#endif
