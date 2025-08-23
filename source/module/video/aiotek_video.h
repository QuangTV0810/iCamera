#ifndef __AIOTEK_VIDEO_H__
#define __AIOTEK_VIDEO_H__

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include "rk_debug.h"
#include "rk_defines.h"
#include "rk_mpi_adec.h"
#include "rk_mpi_aenc.h"
#include "rk_mpi_ai.h"
#include "rk_mpi_ao.h"
#include "rk_mpi_avs.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_ivs.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_rgn.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_tde.h"
#include "rk_mpi_vdec.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_vo.h"
#include "rk_mpi_vpss.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*VideoCallback)(void* pData, uint32_t len, uint64_t pts, void* user_data);

typedef struct {
    int width;              // e.g., 1920
    int height;             // e.g., 1080
    RK_CODEC_ID_E codec;    // e.g., RK_VIDEO_ID_AVC
} VideoConfig_t;

typedef struct {
    VideoConfig_t config;           // Video configuration
    RK_S32 vi_dev;                  // VI device ID (default 0)
    RK_S32 vi_chn;                  // VI channel ID (default 0)
    RK_S32 venc_chn;                // VENC channel ID (default 0)
    bool is_running;                // Thread running status
    pthread_t thread;               // Capture thread
    pthread_mutex_t mutex;          // Mutex for thread-safe
    VideoCallback callback;         // Callback for video frames
    void* user_data;                // User data for callback
    VENC_STREAM_S stream;           // VENC stream
} VideoCapture_t;

VideoCapture_t* VideoCapture_Create(const VideoConfig_t* config);
void VideoCapture_Destroy(VideoCapture_t* handler);
void VideoCapture_Initialize(VideoCapture_t* handler);
void VideoCapture_Deinitialize(VideoCapture_t* handler);
void VideoCapture_Start(VideoCapture_t* handler);
void VideoCapture_Stop(VideoCapture_t* handler);
void VideoCapture_RegisterCallbackVideo(VideoCapture_t* handler, VideoCallback callback, void* user_data);
bool VideoCapture_IsOperation(VideoCapture_t* handler);

#ifdef __cplusplus
}
#endif

#endif /* __AIOTEK_VIDEO_H__ */