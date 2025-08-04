#include "aiotek_rtsp_push.hpp"

namespace aiotek {
namespace stream {
namespace rtsp {

RTSPPushStream::RTSPPushStream(const std::string& input_url, const std::string& output_url)
    : m_input_rtsp_url(input_url), m_output_rtsp_url(output_url), m_is_running(false) {}

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
    if (m_is_running) {
        std::cerr << "[RTSPPushStream] Already running\n";
        return;
    }
    m_is_running = true;
    m_push_stream_thread = std::thread(&RTSPPushStream::threadPushStreamToServer, this);
}

void RTSPPushStream::stop() {
    {
        std::lock_guard<std::mutex> lock(m_push_stream_mutex);
        if (!m_is_running) return;
        m_is_running = false;
    }

    if (m_push_stream_thread.joinable()) {
        m_push_stream_thread.join();
    }
}

bool RTSPPushStream::isRunning() const {
    return m_is_running.load();
}

void RTSPPushStream::threadPushStreamToServer() {
    std::cout << "[RTSPPushStream] Start streaming from " << m_input_rtsp_url
              << " to " << m_output_rtsp_url << "\n";
    getStream();
    std::cout << "[RTSPPushStream] Streaming thread stopped\n";
}

void RTSPPushStream::getStream() {
    AVFormatContext* in_fmt_ctx = nullptr;
    AVFormatContext* out_fmt_ctx = nullptr;
    AVPacket pkt;
    AVDictionary* in_opts = nullptr;
    AVDictionary* out_opts = nullptr;

    av_dict_set(&in_opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&in_opts, "stimeout", "5000000", 0); // 5s timeout

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

    // *** BẮT ĐẦU THAY ĐỔI 1: Khai báo biến theo dõi timestamp ***
    std::vector<int64_t> last_dts_per_stream(in_fmt_ctx->nb_streams, AV_NOPTS_VALUE);
    // Bạn cũng có thể theo dõi PTS nếu cần, nhưng DTS là nguyên nhân chính của lỗi
    // std::vector<int64_t> last_pts_per_stream(in_fmt_ctx->nb_streams, AV_NOPTS_VALUE);

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
            // ... (cleanup code)
            return;
        }
        out_stream->codecpar->codec_tag = 0;
    }

    av_dict_set(&out_opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&out_opts, "muxdelay", "0.1", 0);

    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&out_fmt_ctx->pb, m_output_rtsp_url.c_str(), AVIO_FLAG_WRITE, nullptr, &out_opts) < 0) {
            std::cerr << "[RTSPPushStream] Could not open output URL.\n";
            // ... (cleanup code)
            return;
        }
    }

    if (avformat_write_header(out_fmt_ctx, &out_opts) < 0) {
        std::cerr << "[RTSPPushStream] Error occurred when writing header to output.\n";
        // ... (cleanup code)
        return;
    }
    av_dict_free(&out_opts);

    while (m_is_running.load()) {
        if (av_read_frame(in_fmt_ctx, &pkt) < 0) {
            std::cerr << "[RTSPPushStream] Failed to read frame\n";
            // Thêm logic retry/reconnect ở đây sẽ tốt hơn là chỉ sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            // Cân nhắc việc đóng và mở lại input context nếu lỗi kéo dài
            continue;
        }

        if (pkt.stream_index >= (int)out_fmt_ctx->nb_streams) {
            av_packet_unref(&pkt);
            continue;
        }

        AVStream* in_stream = in_fmt_ctx->streams[pkt.stream_index];
        AVStream* out_stream = out_fmt_ctx->streams[pkt.stream_index];

        // *** BẮT ĐẦU THAY ĐỔI 2: Logic sửa timestamp ***
        
        // 1. Rescale timestamp như cũ
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        // 2. Kiểm tra và sửa lỗi non-monotonic DTS
        if (last_dts_per_stream[pkt.stream_index] != AV_NOPTS_VALUE && pkt.dts <= last_dts_per_stream[pkt.stream_index]) {
            std::cerr << "[RTSPPushStream] Non-monotonic DTS in stream " << pkt.stream_index
                      << ": new DTS " << pkt.dts << " <= last DTS " << last_dts_per_stream[pkt.stream_index]
                      << ". Correcting." << std::endl;
            
            // Lấy lại PTS để duy trì độ chênh lệch (nếu có)
            int64_t pts_dts_diff = pkt.pts - pkt.dts;

            // Sửa DTS: gán nó bằng giá trị cuối cùng + 1
            pkt.dts = last_dts_per_stream[pkt.stream_index] + 1;
            
            // Sửa PTS tương ứng để giữ đồng bộ A/V
            if (pts_dts_diff > 0) {
                 pkt.pts = pkt.dts + pts_dts_diff;
            } else {
                 pkt.pts = pkt.dts;
            }
        }
        
        // 3. Cập nhật giá trị DTS cuối cùng cho stream này
        last_dts_per_stream[pkt.stream_index] = pkt.dts;


        std::cout << "[DEBUG] pts: " << pkt.pts << " dts: " << pkt.dts << std::endl;

        if (av_interleaved_write_frame(out_fmt_ctx, &pkt) < 0) {
            std::cerr << "[RTSPPushStream] Error muxing packet\n";
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

} // namespace rtsp
} // namespace stream
} // namespace aiotek
