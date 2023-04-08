
#define AMRECORDER_IMPORT
#include "../Recorder/export.h"

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib,"Shcore.lib")
#include <ShellScalingApi.h>

#include <string>
#include <iostream>
#include <thread>

void on_preview_image(
	const unsigned char *data,
	unsigned int size,
	int width,
	int height,
	int type) {
	printf("on_preview_image size:%d,width: %d,height: %d type %d\r\n", size,width,height, type);

	static FILE *ff = fopen("scale.yuv", "w+");
	if (ff) {
		fwrite(data, size, 1, ff);
	}
}

void on_split_progress(
	const char* path,
	int index, 
	int over, 
	int error
) {
	printf("on_split_progress path: %s,index: %d,over:%d,error:%d \n",path,index,over,error);
}

void remux_process(const char *path, int progress, int total)
{
	printf("remux_process: %s, %d,%d\n",path,progress,total);
}

void remux_state(const char *path, int state, int error)
{
	printf("remux_state: %s,%d,%d\n",path,state,error);
}

int main()
{
	AMRECORDER_DEVICE *speakers = NULL, *mics = NULL;
	AMRECORDER_ENCODERS *vencoders = NULL;

	AMRECORDER_SETTING setting = {0};
	AMRECORDER_CALLBACK callback = {0};

	char temp_path[MAX_PATH];

	GetTempPathA(MAX_PATH, temp_path);

	std::string log_path(temp_path);

	log_path += "amrecorder.log";

	recorder_set_logpath(log_path.c_str());


	int nspeaker = recorder_get_speakers(&speakers);
	
	int nmic = recorder_get_mics(&mics);

	int n_vencoders = recorder_get_vencoders(&vencoders);

	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

	HWND wnd = ::FindWindow(NULL, L"����");
	if (wnd == nullptr) {
		printf(" not find bixin client window.\n");
		return -1;
	}
	
	RECT rt;
	GetWindowRect(wnd, &rt);
	int width = rt.right - rt.left;
	int height = rt.bottom - rt.top;

	setting.v_handle = wnd;
	setting.v_left = rt.left;//0
	setting.v_top = rt.top;//0
	setting.v_width = width;//GetSystemMetrics(SM_CXSCREEN);
	setting.v_height = height;//GetSystemMetrics(SM_CYSCREEN);
	setting.v_qb = 100;
	setting.v_bit_rate = 1280 * 1000;
	setting.v_frame_rate = 15;
	setting.v_cutting_screen = false;
	setting.v_mouse_track = true;

	//////////////should be the truely encoder id,zero will always be soft x264
	setting.v_enc_id = 0;

	//split record.
	setting.is_split = true;
	setting.split_duration = 4;

	sprintf(setting.output, "save.flv");
	//sprintf(setting.output, "..\\..\\save.mkv");

#if 1 //record speaker
	for (int i = 0; i < nspeaker; i++) {
		if (speakers[i].is_default == 1)
			memcpy(&setting.a_speaker, &speakers[i], sizeof(AMRECORDER_DEVICE));
	}
#endif

#if 1 //record mic
	for (int i = 0; i < nmic; i++) {
		if (mics[i].is_default == 1)
			memcpy(&setting.a_mic, &mics[i], sizeof(AMRECORDER_DEVICE));
	}
#endif

	callback.func_preview_yuv = on_preview_image;
	callback.func_split_progress = on_split_progress;

	printf("please input record seconds:\n");
	int nums = 0;
	scanf("%d", &nums);
	printf("record time : %d\n", nums);

#if 1
	int err = recorder_init(setting, callback);

	recorder_set_preview_enabled(false);

	err = recorder_start();
#endif

#if 0
	std::thread([=]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		recorder_remux(setting.output, "save-back.flv", remux_process, remux_state);
	}).detach();
#endif

	//CAN NOT PAUSE FOR NOW
	/*getchar();

	recorder_pause();

	printf("recorder paused\r\n");

	getchar();

	recorder_resume();

	printf("recorder resumed\r\n");*/

	for (int i = 0; i < nums; i++) {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		printf("record time : sleep 2s  count: %d\n", i);
		if (nums == 2) {
			recorder_remux_destory();
		}
	}

	recorder_stop();

	recorder_release();

	recorder_free_array(speakers);
	recorder_free_array(mics);
	recorder_free_array(vencoders);

	printf("press any key to exit...\r\n");
	system("pause");

	return 0;
}