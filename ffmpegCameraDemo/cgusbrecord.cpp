#include "cgusbrecord.h"
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>


CGusbRecord::CGusbRecord()
    : m_bRecord(false)
{
    m_swsContext = nullptr;
    m_outformat = AV_PIX_FMT_YUV420P;
    m_swsFrame = av_frame_alloc();
}

CGusbRecord::~CGusbRecord()
{
    if(nullptr != m_avformatContext)
    {
        av_write_trailer(m_avformatContext);
        avformat_close_input(&m_avformatContext);
    }

    if (nullptr != m_swsContext)
    {
        sws_freeContext(m_swsContext);
    }

    if (nullptr != m_swsFrame)
    {
        av_frame_free(&m_swsFrame);
    }
}

void CGusbRecord::InitSwsContext()
{
    if (false == m_bRecord) return;
    m_swsContext = sws_getContext(m_width, m_height, m_informat, m_width, m_height, m_outformat,
                                  SWS_BICUBIC, NULL, NULL, NULL);
}

void CGusbRecord::InitSwsFrame()
{
    if (false == m_bRecord) return;

    int numBytes = av_image_get_buffer_size(m_outformat, m_width, m_height, 1);

    uint8_t *pSwpBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(m_swsFrame->data, m_swsFrame->linesize, pSwpBuffer, m_outformat, m_width, m_height, 1);
    m_swsFrame->width = m_width;
    m_swsFrame->height = m_height;
    m_swsFrame->format = m_outformat;
}

/*!
  @brief: 根据视频流创建编码器；采用H264编码格式
*/
int CGusbRecord::InitEncodec()
{
    if (false == m_bRecord) return 0;
    m_informat = AV_PIX_FMT_BGRA;       // 默认传入RGB32图像

    m_avcodec = avcodec_find_encoder(AV_CODEC_ID_H264); //软编码
    m_avcodecContext = avcodec_alloc_context3(m_avcodec);

    m_avcodecContext->time_base = {1, m_fps};
    m_avcodecContext->framerate = {m_fps, 1};
    m_avcodecContext->pix_fmt = m_outformat;    // 默认使用AV_PIX_FMT_YUV420P
    m_avcodecContext->width = m_width;
    m_avcodecContext->height = m_height;
    m_avcodecContext->gop_size = 12;
    m_avcodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    AVDictionary *opt = nullptr;
    av_dict_set(&opt, "video_track_timescale", "25", 0);
    av_opt_set(m_avcodecContext, "tune", "zerolatency", 0);
    int ret = avcodec_open2(m_avcodecContext, m_avcodec, nullptr); //打开解码器
    if (ret < 0)
    {
        char errorstr[AV_ERROR_MAX_STRING_SIZE] = { '\0' };
        qDebug() << "open video codec failed. error:" << av_make_error_string(errorstr, AV_ERROR_MAX_STRING_SIZE, ret);
        return ret;
    }

    return ret;
}

void CGusbRecord::WritePacket(const uint8_t *data)
{
    if (false == m_bRecord) return;

    AVFrame *frame = av_frame_alloc();
    av_image_fill_arrays(frame->data, frame->linesize, data, AV_PIX_FMT_RGB32, m_width, m_height, 1);
    sws_scale(m_swsContext, (const uint8_t *const *)frame->data,
              frame->linesize, 0, m_height, (uint8_t *const *)m_swsFrame->data, m_swsFrame->linesize);
    auto packet = encode(m_swsFrame);
    if(packet)
    {
        packet->pts = packet->dts = m_packetCount * (m_avformatContext->streams[0]->time_base.den) /
                m_avformatContext->streams[0]->time_base.num / m_fps; // 这个地方不处理好的话，录制的文件会播放速度很快

        m_packetCount++;
        av_interleaved_write_frame(m_avformatContext, packet.get());
    }
}

int CGusbRecord::OpenOutput()
{
    if (false == m_bRecord) return 0;
    QString datetime = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString url = QCoreApplication::applicationDirPath() + "/" + datetime + ".mp4";
    int ret = -1;
    do {
        ret = avformat_alloc_output_context2(&m_avformatContext, nullptr, "mp4", url.toStdString().c_str());
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
            break;
        }

        ret = avio_open2(&m_avformatContext->pb, url.toStdString().c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "open avio failed\n");
            break;
        }

        // 为上下文创建一个输出流
        AVStream *stream = avformat_new_stream(m_avformatContext, m_avcodec);
        ret = avcodec_parameters_from_context(stream->codecpar, m_avcodecContext);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_from_context failed.");
            break;
        }

        ret = avformat_write_header(m_avformatContext, nullptr);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "copy coddec context failed");
            break;
        }

        av_log(NULL, AV_LOG_INFO, " Open output file success %s\n", url.toStdString().c_str());
        return ret; // 成功的话是不需要后面的操作的，所以这里就直接return
    } while (0);

    if(nullptr != m_avformatContext)
    {
        avformat_close_input(&m_avformatContext);
    }

    return ret;
}

std::shared_ptr<AVPacket> CGusbRecord::encode(AVFrame *frame)
{
    std::shared_ptr<AVPacket> pkt(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p); });
    av_init_packet(pkt.get());
    pkt->data = NULL;
    pkt->size = 0;
    int ret = avcodec_send_frame(m_avcodecContext, frame);
    ret |= avcodec_receive_packet(m_avcodecContext, pkt.get()); // 这里会返回-11，应该是某个参数设置的有问题
    return (ret >= 0) ? pkt : nullptr;
}
