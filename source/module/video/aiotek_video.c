#include "aiotek_video.h"

RK_U64 TEST_COMM_GetNowUs()
{
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (RK_U64) time.tv_sec * 1000000 + (RK_U64) time.tv_nsec / 1000; /* microseconds */
}

static void* ThreadVideoHandler(void* arg)
{
    VideoCapture_t* handler = (VideoCapture_t*) arg;
    RK_LOGI("VideoCapture: ThreadVideoHandler started");
    int loop_count = 0;

    while (handler->is_running) {
        RK_S32 ret = RK_MPI_VENC_GetStream(handler->venc_chn, &handler->stream, 1000);
        if (ret != RK_SUCCESS) {
            RK_LOGE("VideoCapture: RK_MPI_VENC_GetStream failed: %d", ret);
            pthread_mutex_lock(&handler->mutex);

            pthread_mutex_unlock(&handler->mutex);
            usleep(5000000);
            continue;
        }

        pthread_mutex_lock(&handler->mutex);
        if (handler->callback) {
            void* data = RK_MPI_MB_Handle2VirAddr(handler->stream.pstPack->pMbBlk);
            handler->callback(data, handler->stream.pstPack->u32Len, handler->stream.pstPack->u64PTS, handler->user_data);
        }
        pthread_mutex_unlock(&handler->mutex);

        RK_U64 now_us = TEST_COMM_GetNowUs();
        RK_LOGD("VideoCapture: chn:%d, loopCount:%d, seq:%d, len:%d, pts:%lld, delay:%lldus", handler->venc_chn, loop_count, handler->stream.u32Seq,
                handler->stream.pstPack->u32Len, handler->stream.pstPack->u64PTS, now_us - handler->stream.pstPack->u64PTS);

        ret = RK_MPI_VENC_ReleaseStream(handler->venc_chn, &handler->stream);
        if (ret != RK_SUCCESS) {
            RK_LOGE("VideoCapture: RK_MPI_VENC_ReleaseStream failed: %d", ret);
        }
        loop_count++;
    }

    RK_LOGI("VideoCapture: ThreadVideoHandler stopped");
    return NULL;
}

VideoCapture_t* VideoCapture_Create(const VideoConfig_t* config)
{
    VideoCapture_t* handler = (VideoCapture_t*) malloc(sizeof(VideoCapture_t));
    if (!handler) {
        RK_LOGE("VideoCapture: Failed to allocate memory");
        return NULL;
    }
    memset(handler, 0, sizeof(VideoCapture_t));
    handler->config.width = config->width > 0 ? config->width : 1920;
    handler->config.height = config->height > 0 ? config->height : 1080;
    handler->config.codec = config->codec != 0 ? config->codec : RK_VIDEO_ID_AVC;
    handler->vi_dev = 0;
    handler->vi_chn = 0;
    handler->venc_chn = 0;
    handler->is_running = false;
    handler->callback = NULL;
    handler->user_data = NULL;
    handler->stream.pstPack = NULL;
    pthread_mutex_init(&handler->mutex, NULL);
    RK_LOGI("VideoCapture: Created, resolution=%dx%d, codec=%d", handler->config.width, handler->config.height, handler->config.codec);
    return handler;
}

void VideoCapture_Destroy(VideoCapture_t* handler)
{
    if (!handler)
        return;
    VideoCapture_Deinitialize(handler);
    if (handler->stream.pstPack)
        free(handler->stream.pstPack);
    pthread_mutex_destroy(&handler->mutex);
    free(handler);
    RK_LOGI("VideoCapture: Destroyed");
}

void VideoCapture_Initialize(VideoCapture_t* handler)
{
    RK_LOGI("VideoCapture: Initializing");

    // Initialize RK_MPI
    RK_S32 ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_SYS_Init failed: %d", ret);
        return;
    }

    // Initialize VI device
    VI_DEV_ATTR_S dev_attr;
    VI_DEV_BIND_PIPE_S bind_pipe;
    memset(&dev_attr, 0, sizeof(dev_attr));
    memset(&bind_pipe, 0, sizeof(bind_pipe));
    ret = RK_MPI_VI_GetDevAttr(handler->vi_dev, &dev_attr);
    if (ret == RK_ERR_VI_NOT_CONFIG) {
        ret = RK_MPI_VI_SetDevAttr(handler->vi_dev, &dev_attr);
        if (ret != RK_SUCCESS) {
            RK_LOGE("VideoCapture: RK_MPI_VI_SetDevAttr failed: %d", ret);
            RK_MPI_SYS_Exit();
            return;
        }
    }
    ret = RK_MPI_VI_GetDevIsEnable(handler->vi_dev);
    if (ret != RK_SUCCESS) {
        ret = RK_MPI_VI_EnableDev(handler->vi_dev);
        if (ret != RK_SUCCESS) {
            RK_LOGE("VideoCapture: RK_MPI_VI_EnableDev failed: %d", ret);
            RK_MPI_SYS_Exit();
            return;
        }
        bind_pipe.u32Num = 1;
        bind_pipe.PipeId[0] = handler->vi_dev;
        ret = RK_MPI_VI_SetDevBindPipe(handler->vi_dev, &bind_pipe);
        if (ret != RK_SUCCESS) {
            RK_LOGE("VideoCapture: RK_MPI_VI_SetDevBindPipe failed: %d", ret);
            RK_MPI_SYS_Exit();
            return;
        }
    }

    // Initialize VI channel
    VI_CHN_ATTR_S chn_attr;
    memset(&chn_attr, 0, sizeof(chn_attr));
    chn_attr.stIspOpt.u32BufCount = 2;
    chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
    chn_attr.stSize.u32Width = handler->config.width;
    chn_attr.stSize.u32Height = handler->config.height;
    chn_attr.enPixelFormat = RK_FMT_YUV420SP;
    chn_attr.enCompressMode = COMPRESS_MODE_NONE;
    chn_attr.u32Depth = 0;
    ret = RK_MPI_VI_SetChnAttr(handler->vi_dev, handler->vi_chn, &chn_attr);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VI_SetChnAttr failed: %d", ret);
        RK_MPI_SYS_Exit();
        return;
    }

    // Initialize VENC
    VENC_CHN_ATTR_S venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));
    venc_attr.stVencAttr.enType = handler->config.codec;
    venc_attr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
    if (handler->config.codec == RK_VIDEO_ID_AVC) {
        venc_attr.stVencAttr.u32Profile = H264E_PROFILE_HIGH;
        venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        venc_attr.stRcAttr.stH264Cbr.u32BitRate = 10 * 1024;
        venc_attr.stRcAttr.stH264Cbr.u32Gop = 60;
    } else if (handler->config.codec == RK_VIDEO_ID_HEVC) {
        venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
        venc_attr.stRcAttr.stH265Cbr.u32BitRate = 10 * 1024;
        venc_attr.stRcAttr.stH265Cbr.u32Gop = 60;
    } else if (handler->config.codec == RK_VIDEO_ID_MJPEG) {
        venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
        venc_attr.stRcAttr.stMjpegCbr.u32BitRate = 10 * 1024;
    }
    venc_attr.stVencAttr.u32PicWidth = handler->config.width;
    venc_attr.stVencAttr.u32PicHeight = handler->config.height;
    venc_attr.stVencAttr.u32VirWidth = handler->config.width;
    venc_attr.stVencAttr.u32VirHeight = handler->config.height;
    venc_attr.stVencAttr.u32StreamBufCnt = 2;
    venc_attr.stVencAttr.u32BufSize = handler->config.width * handler->config.height * 3 / 2;
    venc_attr.stVencAttr.enMirror = MIRROR_NONE;
    ret = RK_MPI_VENC_CreateChn(handler->venc_chn, &venc_attr);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VENC_CreateChn failed: %d", ret);
        RK_MPI_SYS_Exit();
        return;
    }

    if (!handler->stream.pstPack) {
        handler->stream.pstPack = (VENC_PACK_S*) malloc(sizeof(VENC_PACK_S));
        if (!handler->stream.pstPack) {
            RK_LOGE("VideoCapture: Failed to allocate stream.pstPack");
            RK_MPI_VENC_DestroyChn(handler->venc_chn);
            RK_MPI_SYS_Exit();
            return;
        }
    }
}

void VideoCapture_Deinitialize(VideoCapture_t* handler)
{
    RK_LOGI("VideoCapture: Deinitializing");
    VideoCapture_Stop(handler);
    RK_S32 ret = RK_MPI_VI_DisableChn(handler->vi_dev, handler->vi_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VI_DisableChn failed: %d", ret);
    }
    ret = RK_MPI_VENC_DestroyChn(handler->venc_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VENC_DestroyChn failed: %d", ret);
    }
    ret = RK_MPI_VI_DisableDev(handler->vi_dev);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VI_DisableDev failed: %d", ret);
    }
    ret = RK_MPI_SYS_Exit();
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_SYS_Exit failed: %d", ret);
    }
    if (handler->stream.pstPack) {
        free(handler->stream.pstPack);
        handler->stream.pstPack = NULL;
    }

    RK_LOGI("VideoCapture: Deinitialized successfully");
}

void VideoCapture_Start(VideoCapture_t* handler)
{
    if (handler->is_running) {
        return;
    }
    RK_S32 ret = RK_MPI_VI_EnableChn(handler->vi_dev, handler->vi_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VI_EnableChn failed: %d", ret);
        return;
    }
    VENC_RECV_PIC_PARAM_S recv_param;
    memset(&recv_param, 0, sizeof(recv_param));
    recv_param.s32RecvPicNum = -1;
    ret = RK_MPI_VENC_StartRecvFrame(handler->venc_chn, &recv_param);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VENC_StartRecvFrame failed: %d", ret);
        RK_MPI_VI_DisableChn(handler->vi_dev, handler->vi_chn);
        return;
    }
    MPP_CHN_S src_chn, dest_chn;
    src_chn.enModId = RK_ID_VI;
    src_chn.s32DevId = handler->vi_dev;
    src_chn.s32ChnId = handler->vi_chn;
    dest_chn.enModId = RK_ID_VENC;
    dest_chn.s32DevId = 0;
    dest_chn.s32ChnId = handler->venc_chn;
    ret = RK_MPI_SYS_Bind(&src_chn, &dest_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_SYS_Bind failed: %d", ret);
        RK_MPI_VI_DisableChn(handler->vi_dev, handler->vi_chn);
        RK_MPI_VENC_StopRecvFrame(handler->venc_chn);
        return;
    }
    handler->is_running = true;
    ret = pthread_create(&handler->thread, NULL, ThreadVideoHandler, handler);
    if (ret != 0) {
        RK_LOGE("VideoCapture: pthread_create failed: %d", ret);
        RK_MPI_SYS_UnBind(&src_chn, &dest_chn);
        RK_MPI_VI_DisableChn(handler->vi_dev, handler->vi_chn);
        RK_MPI_VENC_StopRecvFrame(handler->venc_chn);
        handler->is_running = false;
        return;
    }
    RK_LOGI("VideoCapture: Started");
}

void VideoCapture_Stop(VideoCapture_t* handler)
{
    if (!handler->is_running) {
        return;
    }
    handler->is_running = false;
    pthread_mutex_lock(&handler->mutex);
    pthread_mutex_unlock(&handler->mutex);
    pthread_join(handler->thread, NULL);
    MPP_CHN_S src_chn, dest_chn;
    src_chn.enModId = RK_ID_VI;
    src_chn.s32DevId = handler->vi_dev;
    src_chn.s32ChnId = handler->vi_chn;
    dest_chn.enModId = RK_ID_VENC;
    dest_chn.s32DevId = 0;
    dest_chn.s32ChnId = handler->venc_chn;
    RK_S32 ret = RK_MPI_SYS_UnBind(&src_chn, &dest_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_SYS_UnBind failed: %d", ret);
    }
    ret = RK_MPI_VI_DisableChn(handler->vi_dev, handler->vi_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VI_DisableChn failed: %d", ret);
    }
    ret = RK_MPI_VENC_StopRecvFrame(handler->venc_chn);
    if (ret != RK_SUCCESS) {
        RK_LOGE("VideoCapture: RK_MPI_VENC_StopRecvFrame failed: %d", ret);
    }
    RK_LOGI("VideoCapture: Stopped");
}

void VideoCapture_RegisterCallbackVideo(VideoCapture_t* handler, VideoCallback callback, void* user_data)
{
    pthread_mutex_lock(&handler->mutex);
    handler->callback = callback;
    handler->user_data = user_data;
    pthread_mutex_unlock(&handler->mutex);
    RK_LOGI("VideoCapture: Callback registered");
}

bool VideoCapture_IsOperation(VideoCapture_t* handler)
{
    return handler->is_running;
}

// main.c (for example)
/*
#include "aiotek_video.h"
#include <stdio.h>
#include <stdlib.h>

void VideoCallback(void* pData, uint32_t len, uint64_t pts, void* user_data) {
    FILE* file = (FILE*)user_data;
    fwrite(pData, 1, len, file);
    fflush(file);
    RK_LOGI("Callback: Wrote %u bytes, pts=%llu", len, pts);
}

int main() {
    VideoConfig_t config = {
        .width = 1920,
        .height = 1080,
        .codec = RK_VIDEO_ID_AVC
    };
    VideoCapture_t* handler = VideoCapture_Create(&config);
    if (!handler) {
        fprintf(stderr, "Failed to create VideoCapture\n");
        return -1;
    }

    FILE* out_file = fopen("/tmp/output.h264", "wb");
    VideoCapture_RegisterCallbackVideo(handler, VideoCallback, out_file);
    VideoCapture_Initialize(handler);
    VideoCapture_Start(handler);

    getchar();

    VideoCapture_Stop(handler);
    VideoCapture_Deinitialize(handler);
    VideoCapture_Destroy(handler);
    fclose(out_file);
    return 0;
}
*/