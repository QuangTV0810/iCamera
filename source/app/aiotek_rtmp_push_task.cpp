#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <sys/resource.h>

#include "aiotek_task_list.hpp"
#include "aiotek_logger.hpp"
#include "aiotek_def.hpp"
#include "aiotek_rtmp_push_task.hpp"

namespace aiotek {
namespace app {

PushRTMPTask::PushRTMPTask(const std::string& rtmp_url) : m_rtmp_url(rtmp_url), m_ifmt_ctx(nullptr), m_ofmt_ctx(nullptr), m_is_pushing(false)
{
}

PushRTMPTask::~PushRTMPTask()
{
    Deinitialize();
}

void PushRTMPTask::Initialize()
{
    try {
        AVFormatContext* ifmt_ctx = m_ifmt_ctx;
        if (!ifmt_ctx || ifmt_ctx->nb_streams == 0) {
            throw std::runtime_error("m_ifmt_ctx is null or has no streams");
        }

        int ret = avformat_alloc_output_context2(&m_ofmt_ctx, nullptr, "flv", m_rtmp_url.c_str());
        if (ret < 0 || !m_ofmt_ctx) {
            throw std::runtime_error("Failed to alloc output context");
        }

        for (unsigned i = 0; i < ifmt_ctx->nb_streams; ++i) {
            AVStream* in_stream = ifmt_ctx->streams[i];
            if (!in_stream || !in_stream->codecpar) {
                throw std::runtime_error("Invalid input stream");
            }
            AVStream* out_stream = avformat_new_stream(m_ofmt_ctx, nullptr);
            if (!out_stream) {
                throw std::runtime_error("Failed to create output stream");
            }
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                throw std::runtime_error("Failed to copy codecpar");
            }
            out_stream->time_base = in_stream->time_base;
        }

        if (!(m_ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&m_ofmt_ctx->pb, m_rtmp_url.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                avformat_free_context(m_ofmt_ctx);
                m_ofmt_ctx = nullptr;
                throw std::runtime_error("Failed to open output URL");
            }
        }

        ret = avformat_write_header(m_ofmt_ctx, nullptr);
        if (ret < 0) {
            avio_closep(&m_ofmt_ctx->pb);
            avformat_free_context(m_ofmt_ctx);
            m_ofmt_ctx = nullptr;
            throw std::runtime_error("Failed to write header");
        }

        m_prev_dts.resize(ifmt_ctx->nb_streams, AV_NOPTS_VALUE);
    } catch (const std::exception& e) {
        std::cout << "PushRTMPTask: Initialize failed: " << e.what() << std::endl;
        if (m_ofmt_ctx) {
            avformat_free_context(m_ofmt_ctx);
            m_ofmt_ctx = nullptr;
        }
        throw;
    }
}

void PushRTMPTask::Deinitialize()
{
    if (m_is_pushing) {
        Stop();
    }
    if (m_ofmt_ctx) {
        av_write_trailer(m_ofmt_ctx);
        if (!(m_ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&m_ofmt_ctx->pb);
        }
        avformat_free_context(m_ofmt_ctx);
        m_ofmt_ctx = nullptr;
    }
}

void PushRTMPTask::Start()
{
    if (m_is_pushing) {
        return;
    }
    m_is_pushing = true;
}

void PushRTMPTask::Stop()
{
    if (!m_is_pushing) {
        return;
    }
    m_is_pushing = false;
}

void PushRTMPTask::ThreadPushRTMPHandler(aiotek::core::Task& task)
{
    auto media_ring_buffer = aiotek::core::RingBufferManager::GetInstance().GetRingBuffer("push_media_to_zlm");
    try {
        while (task.IsOperation()) {
            if (task.IsSuspended()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            auto packet = task.GetMailbox().receive(std::chrono::milliseconds(1));
            if (packet) {
                if (RTSP_TASK == packet->task_sender_id) {
                    auto sig = static_cast<aiotek::common::RTMPSignal>(packet->signal);
                    AIOTEK_LOG_INFO("Task: " << task.GetName() << " received mailbox from: " << packet->task_sender_id);
                    switch (sig) {
                        case aiotek::common::RTMPSignal::RTSP_INIT_SUCCUSS_SIG: {
                            m_ifmt_ctx = std::any_cast<AVFormatContext*>(packet->msg);
                            if (m_ifmt_ctx && m_ifmt_ctx->nb_streams > 0) {
                                this->Initialize();
                                this->Start();
                            } else {
                            }
                            break;
                        }
                        case aiotek::common::RTMPSignal::RTMP_START_PUSH_SIG: {
                            this->Start();
                            break;
                        }
                        case aiotek::common::RTMPSignal::RTMP_STOP_PUSH_SIG: {
                            this->Stop();
                            break;
                        }
                        default:
                            break;
                    }
                } else {
                }
            }

            if (m_is_pushing && m_ofmt_ctx && m_ifmt_ctx) {
                AVPacket pkt;
                if (nullptr != media_ring_buffer) {
                    auto packetOpt = media_ring_buffer->pop(std::chrono::milliseconds(50));
                    if (!packetOpt) {
                        continue;
                    } else {
                        pkt = packetOpt.value();
                    }
                } else {
                    media_ring_buffer = aiotek::core::RingBufferManager::GetInstance().GetRingBuffer("push_media_to_zlm");
                    continue;
                }

                if (pkt.stream_index < 0 || static_cast<unsigned>(pkt.stream_index) >= m_ifmt_ctx->nb_streams ||
                    static_cast<unsigned>(pkt.stream_index) >= m_ofmt_ctx->nb_streams) {
                    av_packet_unref(&pkt);
                    continue;
                }

                AVStream* in_stream = m_ifmt_ctx->streams[pkt.stream_index];
                AVStream* out_stream = m_ofmt_ctx->streams[pkt.stream_index];
                if (!in_stream || !out_stream) {
                    av_packet_unref(&pkt);
                    continue;
                }

                if (pkt.pts == AV_NOPTS_VALUE && pkt.dts != AV_NOPTS_VALUE) {
                    pkt.pts = pkt.dts;
                }

                pkt.pts =
                    av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt.dts =
                    av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
                pkt.pos = -1;

                if (m_prev_dts[pkt.stream_index] != AV_NOPTS_VALUE && pkt.dts <= m_prev_dts[pkt.stream_index]) {
                    pkt.dts = m_prev_dts[pkt.stream_index] + (pkt.duration > 0 ? pkt.duration : 1);
                    pkt.pts = std::max(pkt.pts, pkt.dts);
                }
                m_prev_dts[pkt.stream_index] = pkt.dts;

                int ret = -1;
                for (int retries = 0; retries < 3 && m_is_pushing; ++retries) {
                    ret = av_interleaved_write_frame(m_ofmt_ctx, &pkt);
                    if (ret >= 0) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                if (ret < 0) {
                }

                av_packet_unref(&pkt);
            } else {
            }
        }
    } catch (const std::exception& e) {
        std::cout << "PushRTMPTask: Exception in handler: " << e.what() << std::endl;
    }
}

} // namespace app
} // namespace aiotek