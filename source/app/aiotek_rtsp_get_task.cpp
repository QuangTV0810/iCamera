#include "aiotek_rtsp_get_task.hpp"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <sys/resource.h>
#include "aiotek_task.hpp"
#include "aiotek_mailbox.hpp"
#include "aiotek_task_list.hpp"
#include "aiotek_def.hpp"
#include "aiotek_ring_buffer.hpp"

namespace aiotek {
namespace app {

GetRTSPTask::GetRTSPTask(const std::string& rtsp_url) : m_rtsp_url(rtsp_url), m_ifmt_ctx(nullptr)
{
    aiotek::core::RingBufferManager::GetInstance().CreateRingBuffer("push_media_to_zlm", 32);

    Initialize();
}

GetRTSPTask::~GetRTSPTask()
{
    Deinitialize();
}

void GetRTSPTask::Initialize()
{
    try {
        if (avformat_network_init() < 0) {
            throw std::runtime_error("Failed to initialize network");
        }

        int ret = -1;
        AVDictionary* opts = nullptr;

        for (int retries = 0; retries < 3; ++retries) {
            ret = avformat_open_input(&m_ifmt_ctx, m_rtsp_url.c_str(), nullptr, nullptr);
            if (ret >= 0 && m_ifmt_ctx) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        av_dict_free(&opts);
        if (ret < 0 || !m_ifmt_ctx) {
            avformat_free_context(m_ifmt_ctx);
            m_ifmt_ctx = nullptr;
            throw std::runtime_error("Failed to open RTSP input");
        }

        ret = avformat_find_stream_info(m_ifmt_ctx, nullptr);
        if (ret < 0 || m_ifmt_ctx->nb_streams == 0) {
            avformat_close_input(&m_ifmt_ctx);
            m_ifmt_ctx = nullptr;
            throw std::runtime_error("Failed to find stream info");
        }

        aiotek::core::MailboxPacket packet{.task_sender_id = RTSP_TASK,
                                           .task_receiver_id = RTMP_TASK,
                                           .signal = static_cast<int32_t>(aiotek::common::RTMPSignal::RTSP_INIT_SUCCUSS_SIG),
                                           .msg = m_ifmt_ctx,
                                           .len = sizeof(AVFormatContext*)};

        auto rtmp_mailbox = aiotek::core::TaskManager::GetMailbox(RTMP_TASK);
        if (nullptr != rtmp_mailbox) {
            bool status = rtmp_mailbox->send(packet);
            if (status) {
                AIOTEK_LOG_INFO("GetRTSPTask: Sent m_ifmt_ctx to RTMP_TASK");
            } else {
                AIOTEK_LOG_ERROR("GetRTSPTask: Failed to send m_ifmt_ctx to RTMP_TASK");
            }
        } else {
            AIOTEK_LOG_ERROR("GetRTSPTask: No mailbox for RTMP_TASK");
        }

    } catch (const std::exception& e) {
        AIOTEK_LOG_ERROR("GetRTSPTask: Initialize failed: " << e.what());
        if (m_ifmt_ctx) {
            avformat_close_input(&m_ifmt_ctx);
            m_ifmt_ctx = nullptr;
        }
        throw;
    }
}

void GetRTSPTask::Deinitialize()
{
    if (m_ifmt_ctx) {
        avformat_close_input(&m_ifmt_ctx);
        m_ifmt_ctx = nullptr;
    }
    avformat_network_deinit();
}

void GetRTSPTask::ThreadGetRTSPHandler(aiotek::core::Task& task)
{
    try {
        if (!m_ifmt_ctx || m_ifmt_ctx->nb_streams == 0) {
            AIOTEK_LOG_ERROR( "GetRTSPTask: m_ifmt_ctx is null or has no streams");
            return;
        }
        auto media_ring_buffer = aiotek::core::RingBufferManager::GetInstance().GetRingBuffer("push_media_to_zlm");
        if (!media_ring_buffer) {
            AIOTEK_LOG_ERROR("GetRTSPTask: cannot get media_ring_buffer");
            return;
        }

        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;

        while (task.IsOperation()) {
            if (task.IsSuspended()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            int ret = -1;
            for (int retries = 0; retries < 3; ++retries) {
                ret = av_read_frame(m_ifmt_ctx, &pkt);
                if (ret >= 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (ret < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            if (media_ring_buffer->push(pkt)) {
            } else {
            }
            av_packet_unref(&pkt);
        }
    } catch (const std::exception& e) {
        AIOTEK_LOG_ERROR("GetRTSPTask: Exception in handler: " << e.what());
    }
}

} // namespace app
} // namespace aiotek