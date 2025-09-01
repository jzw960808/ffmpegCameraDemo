#ifndef CGUSBRECORD_H
#define CGUSBRECORD_H
#include <QThread>

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
}

class CGusbRecord
{
public:
    CGusbRecord();
    virtual ~CGusbRecord();

    void SetRecordFlag(bool flag) { m_bRecord = flag; }
    inline void SetResolution(int width, int height, int fps)
    {
        m_width = width;
        m_height = height;
        m_fps = fps;
    }

    void InitSwsContext();
    void InitSwsFrame();
    int InitEncodec();
    int OpenOutput();
    void WritePacket(const uint8_t *data);

private:
    std::shared_ptr<AVPacket> encode(AVFrame * frame);

private:
    AVFormatContext *m_avformatContext;
    const AVCodec *m_avcodec;
    AVCodecContext *m_avcodecContext;
    SwsContext *m_swsContext;
    AVFrame *m_swsFrame;
    bool m_bRecord;

    int m_width;
    int m_height;
    int m_fps;
    AVPixelFormat m_informat;
    AVPixelFormat m_outformat;
    int64_t m_packetCount = 0;
};

#endif // CGUSBRECORD_H
