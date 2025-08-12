#include "aiotek_rtmp_push.hpp"

namespace aiotek {
namespace module {
namespace stream {
namespace rtmp {

static std::string avErr2Str(int errnum)
{
    char buf[256];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

RTMPPushStream::RTMPPushStream(const std::string& input_url, const std::string& output_url)
    : m_input_rtsp_url(input_url), m_output_rtmp_url(output_url)
{
}

RTMPPushStream::~RTMPPushStream()
{
    stop();
}

void RTMPPushStream::init()
{
    avformat_network_init();
}

void RTMPPushStream::deinit()
{
    avformat_network_deinit();
}

bool RTMPPushStream::isOperation() const
{
    return m_is_stream_running.load();
}

void RTMPPushStream::start()
{
    if (m_is_stream_running.exchange(true))
        return;

    m_thread_need_stop = false;
    m_push_stream_thread = std::thread(&RTMPPushStream::threadPushStream, this);
}

void RTMPPushStream::stop()
{
    if (!m_is_stream_running.exchange(false))
        return;

    m_thread_need_stop = true;
    if (m_push_stream_thread.joinable())
        m_push_stream_thread.join();
}

void RTMPPushStream::threadPushStream()
{
    std::lock_guard<std::mutex> lock(m_push_stream_mutex);
    setupStream();
}

void RTMPPushStream::setupStream()
{
    AVDictionary* opts = nullptr;
    int ret = 0;

    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&opts, "stimeout", "5000000", 0);

    ret = avformat_open_input(&m_ifmt_ctx, m_input_rtsp_url.c_str(), nullptr, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        std::cerr << "[RTMPPush] Failed to open input: " << avErr2Str(ret) << "\n";
        return;
    }
    m_ifmt_ctx->flags |= AVFMT_FLAG_GENPTS;
    ret = avformat_find_stream_info(m_ifmt_ctx, nullptr);
    if (ret < 0) {
        std::cerr << "[RTMPPush] Failed to find stream info: " << avErr2Str(ret) << "\n";
        avformat_close_input(&m_ifmt_ctx);
        return;
    }

    ret = avformat_alloc_output_context2(&m_ofmt_ctx, nullptr, "flv", m_output_rtmp_url.c_str());
    if (ret < 0 || !m_ofmt_ctx) {
        std::cerr << "[RTMPPush] Failed to alloc output context\n";
        avformat_close_input(&m_ifmt_ctx);
        return;
    }

    for (unsigned i = 0; i < m_ifmt_ctx->nb_streams; ++i) {
        AVStream* in_stream = m_ifmt_ctx->streams[i];
        AVStream* out_stream = avformat_new_stream(m_ofmt_ctx, nullptr);
        if (!out_stream) {
            std::cerr << "[RTMPPush] Failed to create output stream\n";
            continue;
        }
        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if (ret < 0) {
            std::cerr << "[RTMPPush] copy codecpar error: " << avErr2Str(ret) << "\n";
        }
        out_stream->time_base = in_stream->time_base;
    }

    if (!(m_ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_ofmt_ctx->pb, m_output_rtmp_url.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "[RTMPPush] Failed to open output URL: " << avErr2Str(ret) << "\n";
            avformat_free_context(m_ofmt_ctx);
            avformat_close_input(&m_ifmt_ctx);
            return;
        }
    }

    ret = avformat_write_header(m_ofmt_ctx, nullptr);
    if (ret < 0) {
        std::cerr << "[RTMPPush] Failed to write header: " << avErr2Str(ret) << "\n";
        avio_closep(&m_ofmt_ctx->pb);
        avformat_free_context(m_ofmt_ctx);
        avformat_close_input(&m_ifmt_ctx);
        return;
    }

    // Khai báo prev_dts LÊN TRÊN, ngay sau khi mở xong stream
    std::vector<int64_t> prev_dts(m_ifmt_ctx->nb_streams, AV_NOPTS_VALUE);

    AVPacket pkt;
    while (!m_thread_need_stop.load()) {
        ret = av_read_frame(m_ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        AVStream* in_stream = m_ifmt_ctx->streams[pkt.stream_index];
        AVStream* out_stream = m_ofmt_ctx->streams[pkt.stream_index];

        // Nếu PTS/DTS chưa có giá trị, tạo mới dựa trên prev_dts
        if (pkt.pts == AV_NOPTS_VALUE) {
            pkt.pts = (prev_dts[pkt.stream_index] == AV_NOPTS_VALUE) ? 0 : prev_dts[pkt.stream_index] + 1;
        }
        if (pkt.dts == AV_NOPTS_VALUE) {
            pkt.dts = pkt.pts;
        }

        // Rescale timestamps
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        // Fix non-monotonic DTS
        if (prev_dts[pkt.stream_index] != AV_NOPTS_VALUE && pkt.dts <= prev_dts[pkt.stream_index]) {
            pkt.dts = prev_dts[pkt.stream_index] + (pkt.duration > 0 ? pkt.duration : 1);
            pkt.pts = std::max(pkt.pts, pkt.dts); // đảm bảo pts >= dts
        }

        prev_dts[pkt.stream_index] = pkt.dts;

        ret = av_interleaved_write_frame(m_ofmt_ctx, &pkt);
        if (ret < 0) {
            std::cerr << "[RTMPPush] Write error: " << avErr2Str(ret) << "\n";
            av_packet_unref(&pkt);
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(m_ofmt_ctx);
    if (!(m_ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&m_ofmt_ctx->pb);

    avformat_free_context(m_ofmt_ctx);
    avformat_close_input(&m_ifmt_ctx);

    std::cerr << "[RTMPPush] Stream stopped\n";
}

} // namespace rtmp
} // namespace stream
} // namespace module
} // namespace aiotek
