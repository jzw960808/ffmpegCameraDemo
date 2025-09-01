#ifndef CGUSBCAMERA_H
#define CGUSBCAMERA_H

#include <QWidget>
#include <string>
#include <memory>
#include <QThread>
#include <iostream>
#include <iostream>
#include <QDebug>
#include <QMutex>
#include <QProcess>
#include <QTextCodec>
#include <QDateTime>
#include <QCoreApplication>
#include "cgusbrecord.h"

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

using namespace std;
class CGUsbCamera : public QThread
{
    Q_OBJECT

    enum DeviceType {
        DT_Video = 0,
        DT_Audio
    };

public:
    explicit CGUsbCamera(QObject *eventReceiver, QString cameName = "", QThread *parent = nullptr);
    ~CGUsbCamera();

    inline void SetcameraName(QString cameName)
    { //设置摄像头名称
        m_cameraName = cameName;
    }

    inline void SetResolution(int width, int height, int fps)
    { //设置摄像头分辨率和帧率，注意摄像头必须支持该分辨率和帧率
        m_width = width;
        m_height = height;
        m_fps = fps;
    }

    void isCapture(bool is);
    QMap<QString,QString> EnumVideoDevices();                   // 获取计算机有效摄像头信息
    QMap<QString,QString> EnumAudioDevices();                   // 获取计算机音频设备信息
    QStringList EnumResolutions(QString cameraName);      // 获取摄像头分辨率

protected:
    void run();

private:
    QString ffmpegCMDexec(QString cmd);       // 执行ffmpeg命令行,win
    QString ffmpegCMDexecLinux(QString cmd);  // 执行ffmpeg命令行,linux
    void lookupDevices(const QString &deviceInfos, QMap<QString, QString> &devices, DeviceType type);
    int openCamera(string inputUrl);

    static int interrupt_cb(void *ctx);
    shared_ptr<AVPacket> readPacketFromSource();

    int initDecodec(AVStream *inputStream);
    bool decode(AVPacket* packet, AVFrame *frame);

    QString gbkToUnicode(const char* chars);    // 转换编码，在Qt程序里显示中文
    QByteArray gbkFromUnicode(QString str);     // Qt输出字符串到外部程序，在外部程序显示中文

private:
    QObject *m_eventReceiver;
    bool bCapture;                              // 是否采集
    QMutex bCaptureMutex;
    QString m_cameraName;                       // 摄像头名称

    AVFormatContext *m_pInputavFormatContext = nullptr; //FFMPEG所有的操作都要通过这个AVFormatContext来进行
    AVCodecContext *m_decodec = nullptr;
    static int64_t lastReadPacktTime ;
    struct SwsContext* m_swsContext = nullptr;

    int m_width;    //当前使用分辨率宽
    int m_height;   //当前使用分辨率高
    int m_fps;      //当前使用帧率
};

#endif // CGUSBCAMERA_H
