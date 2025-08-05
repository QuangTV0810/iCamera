#include "aiotek_rtsp_push.hpp"

namespace aiotek {
namespace stream {
namespace rtsp {

RTSPPushStream::RTSPPushStream(const std::string& input_url, const std::string& output_url)
    : m_input_rtsp_url(input_url), m_output_rtsp_url(output_url), m_is_thread_running(false) {
    // Initialize last DTS tracking for each stream
    for (int i = 0; i < MAX_STREAMS; i++) {
        m_last_dts[i] = AV_NOPTS_VALUE;
        m_last_pts[i] = AV_NOPTS_VALUE;
    }
}

RTSPPushStream::~RTSPPushStream() {
    stop();
    deinit();
}

void RTSPPushStream::init() {
    avformat_network_init();
}

void RTSPPushStream::deinit() {
    avformat_network_deinit();
}

void RTSPPushStream::start() {
    std::lock_guard<std::mutex> lock(m_push_stream_mutex);
    if (m_is_thread_running) {
        std::cerr << "[RTSPPushStream] Already running\n";
        return;
    }
    m_is_thread_running = true;
    m_push_stream_thread = std::thread(&RTSPPushStream::threadPushStreamToServer, this);
}

void RTSPPushStream::stop() {
    {
        std::lock_guard<std::mutex> lock(m_push_stream_mutex);
        if (!m_is_thread_running) return;
        m_is_thread_running = false;
    }

    if (m_push_stream_thread.joinable()) {
        m_push_stream_thread.join();
    }
}

bool RTSPPushStream::isRunning() const {
    return m_is_thread_running.load();
}

void RTSPPushStream::threadPushStreamToServer() {
    std::cout << "[RTSPPushStream] Start streaming from " << m_input_rtsp_url
              << " to " << m_output_rtsp_url << "\n";
    getStream();
    m_is_thread_running = false;
    std::cout << "[RTSPPushStream] Streaming thread stopped\n";
}

void RTSPPushStream::getStream() {
    AVFormatContext* in_fmt_ctx = nullptr;
    AVFormatContext* out_fmt_ctx = nullptr;
    AVPacket pkt;
    AVDictionary* in_opts = nullptr;
    AVDictionary* out_opts = nullptr;

    // Input options - more robust settings
    av_dict_set(&in_opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&in_opts, "stimeout", "5000000", 0);
    av_dict_set(&in_opts, "fflags", "genpts+igndts+discardcorrupt+nobuffer", 0);
    av_dict_set(&in_opts, "max_delay", "500000", 0);
    av_dict_set(&in_opts, "analyzeduration", "1000000", 0);
    av_dict_set(&in_opts, "reorder_queue_size", "0", 0);
    av_dict_set(&in_opts, "buffer_size", "1024000", 0); // 1MB buffer

    if (avformat_open_input(&in_fmt_ctx, m_input_rtsp_url.c_str(), nullptr, &in_opts) < 0) {
        std::cerr << "[RTSPPushStream] Could not open input stream.\n";
        av_dict_free(&in_opts);
        return;
    }
    av_dict_free(&in_opts);

    if (avformat_find_stream_info(in_fmt_ctx, nullptr) < 0) {
        std::cerr << "[RTSPPushStream] Failed to get input stream info.\n";
        avformat_close_input(&in_fmt_ctx);
        return;
    }

    avformat_alloc_output_context2(&out_fmt_ctx, nullptr, "rtsp", m_output_rtsp_url.c_str());
    if (!out_fmt_ctx) {
        std::cerr << "[RTSPPushStream] Could not create output context\n";
        avformat_close_input(&in_fmt_ctx);
        return;
    }

    // Initialize DTS tracking for this session
    for (int i = 0; i < MAX_STREAMS; i++) {
        m_last_dts[i] = AV_NOPTS_VALUE;
        m_last_pts[i] = AV_NOPTS_VALUE;
    }

    for (unsigned int i = 0; i < in_fmt_ctx->nb_streams; ++i) {
        AVStream* in_stream = in_fmt_ctx->streams[i];
        AVStream* out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
        if (!out_stream) {
            std::cerr << "[RTSPPushStream] Failed to allocate output stream\n";
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            return;
        }

        if (avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar) < 0) {
            std::cerr << "[RTSPPushStream] Failed to copy codec parameters\n";
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            return;
        }
        out_stream->codecpar->codec_tag = 0;
        out_stream->time_base = in_stream->time_base;
    }

    // Output options with better timestamp handling
    av_dict_set(&out_opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&out_opts, "muxdelay", "0.1", 0);
    av_dict_set(&out_opts, "fflags", "genpts+autobsf", 0); // Remove igndts
    av_dict_set(&out_opts, "avoid_negative_ts", "make_zero", 0);
    av_dict_set(&out_opts, "max_interleave_delta", "1000000", 0);
    av_dict_set(&out_opts, "use_wallclock_as_timestamps", "1", 0); // Use wallclock timestamps

    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&out_fmt_ctx->pb, m_output_rtsp_url.c_str(), AVIO_FLAG_WRITE, nullptr, &out_opts) < 0) {
            std::cerr << "[RTSPPushStream] Could not open output URL.\n";
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            av_dict_free(&out_opts);
            return;
        }
    }

    if (avformat_write_header(out_fmt_ctx, &out_opts) < 0) {
        std::cerr << "[RTSPPushStream] Error occurred when writing header to output.\n";
        avformat_close_input(&in_fmt_ctx);
        if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&out_fmt_ctx->pb);
        }
        avformat_free_context(out_fmt_ctx);
        av_dict_free(&out_opts);
        return;
    }
    av_dict_free(&out_opts);

    while (m_is_thread_running.load()) {
        if (av_read_frame(in_fmt_ctx, &pkt) < 0) {
            std::cerr << "[RTSPPushStream] Failed to read frame\n";
            
            if (!m_is_thread_running.load()) {
                std::cout << "[RTSPPushStream] Stopping during retry\n";
                break;
            }
            
            std::cerr << "[RTSPPushStream] Attempting to reconnect...\n";
            avformat_close_input(&in_fmt_ctx);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            
            // Reset DTS tracking on reconnect
            for (int i = 0; i < MAX_STREAMS; i++) {
                m_last_dts[i] = AV_NOPTS_VALUE;
                m_last_pts[i] = AV_NOPTS_VALUE;
            }
            
            if (avformat_open_input(&in_fmt_ctx, m_input_rtsp_url.c_str(), nullptr, nullptr) < 0) {
                std::cerr << "[RTSPPushStream] Failed to reconnect\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                continue;
            }
            
            if (avformat_find_stream_info(in_fmt_ctx, nullptr) < 0) {
                std::cerr << "[RTSPPushStream] Failed to get stream info after reconnect\n";
                continue;
            }
            
            std::cout << "[RTSPPushStream] Reconnected successfully\n";
            continue;
        }

        if (pkt.stream_index >= (int)out_fmt_ctx->nb_streams || pkt.stream_index >= MAX_STREAMS) {
            av_packet_unref(&pkt);
            continue;
        }

        AVStream* in_stream = in_fmt_ctx->streams[pkt.stream_index];
        AVStream* out_stream = out_fmt_ctx->streams[pkt.stream_index];

        // Rescale timestamps
        int64_t orig_pts = pkt.pts;
        int64_t orig_dts = pkt.dts;

        if (pkt.pts != AV_NOPTS_VALUE) {
            pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                       (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        }
        
        if (pkt.dts != AV_NOPTS_VALUE) {
            pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                       (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        }
        
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        // Fix non-monotonic DTS
        if (fixTimestamps(&pkt, pkt.stream_index)) {
            std::cout << "[DEBUG] Fixed timestamps for stream " << pkt.stream_index 
                      << " orig_pts: " << orig_pts << " orig_dts: " << orig_dts
                      << " new_pts: " << pkt.pts << " new_dts: " << pkt.dts << "\n";
        }

        if (av_interleaved_write_frame(out_fmt_ctx, &pkt) < 0) {
            std::cerr << "[RTSPPushStream] Error muxing packet for stream " << pkt.stream_index << "\n";
        }

        av_packet_unref(&pkt);
    }

    av_write_trailer(out_fmt_ctx);
    avformat_close_input(&in_fmt_ctx);
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb);
    }
    avformat_free_context(out_fmt_ctx);
}

bool RTSPPushStream::fixTimestamps(AVPacket* pkt, int stream_index) {
    bool fixed = false;
    
    // Fix DTS monotonicity
    if (pkt->dts != AV_NOPTS_VALUE) {
        if (m_last_dts[stream_index] != AV_NOPTS_VALUE && pkt->dts <= m_last_dts[stream_index]) {
            pkt->dts = m_last_dts[stream_index] + 1;
            fixed = true;
        }
        m_last_dts[stream_index] = pkt->dts;
    }
    
    // Fix PTS - ensure PTS >= DTS
    if (pkt->pts != AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE) {
        if (pkt->pts < pkt->dts) {
            pkt->pts = pkt->dts;
            fixed = true;
        }
    } else if (pkt->pts == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE) {
        // Set PTS = DTS if PTS is missing
        pkt->pts = pkt->dts;
        fixed = true;
    }
    
    if (pkt->pts != AV_NOPTS_VALUE) {
        m_last_pts[stream_index] = pkt->pts;
    }
    
    return fixed;
}

} // namespace rtsp
} // namespace stream
} // namespace aiotek