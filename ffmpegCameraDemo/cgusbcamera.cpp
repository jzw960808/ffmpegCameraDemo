#include "cgusbcamera.h"
#include "cgusbcameracustomevent.h"

int64_t CGUsbCamera::lastReadPacktTime = 0;
CGUsbCamera::CGUsbCamera(QObject *eventReceiver, QString cameName, QThread *parent) :
    QThread(parent),
    m_cameraName(cameName),
    m_eventReceiver(eventReceiver)
{
    bCapture = false;

    avformat_network_init();        // 初始化FFmpeg网络模块
    avdevice_register_all();        // 初始化libavdevice并注册所有输入和输出设备
    av_log_set_level(AV_LOG_ERROR);
}

CGUsbCamera::~CGUsbCamera()
{
    if(nullptr != m_pInputavFormatContext) avformat_close_input(&m_pInputavFormatContext);
    if(nullptr != m_swsContext) sws_freeContext(m_swsContext);
}

/*static*/ int CGUsbCamera::interrupt_cb(void *ctx)
{
    int  timeout  = 3;
    if(av_gettime() - lastReadPacktTime > timeout * 1000 * 1000)
    {
        return -1;
    }

    return 0;
}

int CGUsbCamera::openCamera(string inputUrl)
{
    m_pInputavFormatContext = avformat_alloc_context(); // 分配一个AVFormatContext,查找用于输入的设备
    lastReadPacktTime = av_gettime();
    m_pInputavFormatContext->interrupt_callback.callback = interrupt_cb;
    // 使用libavdevice的时候，唯一的不同在于需要首先查找用于输入的设备
#ifdef Q_OS_LINUX
    AVInputFormat *ifmt = av_find_input_format("video4linux2");
#endif

#ifdef Q_OS_WIN32
    const AVInputFormat *ifmt = av_find_input_format("dshow");
#endif

    AVDictionary *format_opts =  nullptr;
    // 只能设置摄像头支持的分辨率和帧率，否则打开摄像头失败，软件崩溃
    QString resolution = QString("%1x%2").arg(m_width).arg(m_height);
    av_dict_set(&format_opts, "video_size", resolution.toLatin1().data(), 0); // 640x480，设置分辨率
    av_dict_set(&format_opts, "framerate", QString::number(m_fps).toLatin1().data(), 0); // 设置帧率
    av_dict_set_int(&format_opts, "rtbufsize", 18432000, 0);

    int ret = 0;
    do {
        if ((ret = avformat_open_input(&m_pInputavFormatContext, inputUrl.c_str(), ifmt, &format_opts)) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
            break;
        }

        if ((ret = avformat_find_stream_info(m_pInputavFormatContext, nullptr)) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
            break;
        }

        av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n",inputUrl.c_str());
    } while(0);

    return ret;
}

shared_ptr<AVPacket> CGUsbCamera::readPacketFromSource()
{
    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    lastReadPacktTime = av_gettime();
    int ret = av_read_frame(m_pInputavFormatContext, packet.get());

    return (ret >= 0) ? packet : nullptr;
}

int CGUsbCamera::initDecodec(AVStream *inputStream)
{
    int ret = -1;
    do {
        auto codecId = inputStream->codecpar->codec_id;
        auto codec = avcodec_find_decoder(codecId);
        if (nullptr == codec) break;

        m_decodec = avcodec_alloc_context3(codec);
        if (nullptr == m_decodec) break;

        avcodec_parameters_to_context(m_decodec, inputStream->codecpar); // 将codecpar中的参数复制到codec中;相应的还有一个函数avcodec_parameters_from_context()
        ret = avcodec_open2(m_decodec, codec, NULL); // 打开解码器
    } while (0);

    return ret;
}

bool CGUsbCamera::decode(AVPacket* packet, AVFrame *frame)
{
    // 解码一帧视频数据
    auto hr = avcodec_send_packet(m_decodec, packet);
    hr |= avcodec_receive_frame(m_decodec, frame);
    return (hr >= 0);
}

void CGUsbCamera::lookupDevices(const QString &deviceInfos, QMap<QString, QString> &devices, DeviceType type)
{
    const char *deviceTypes[] = { "video", "audio" };
    QStringList list = deviceInfos.split("\r\n", QString::SkipEmptyParts);
    //检测到视频设备
    int idx = 0;
    do {
        bool flag = false;
        QString name;
        for (; idx < list.size(); ++idx)
        {
            if (false == list.at(idx).contains("dshow") || false == list.at(idx).contains(deviceTypes[type]))
            {
                continue;
            }

            QRegExp regexp(R"(.*\"(.*)\".*)");
            if (true == regexp.exactMatch(list.at(idx)))
            {
                flag = true;
                name = regexp.cap(1);
                break;  // 查找到一个退出去, 因为一个摄像头有两行数据；如果后续有更好的方案，可以改进
            }
        }

        idx++;
        QRegExp regexp2(R"(.*\"(.*)\".*)");
        if (true == flag && true == regexp2.exactMatch(list.at(idx)))
        {
            devices.insert(name, regexp2.cap(1));
        }

        if (idx >= list.size()) break; // 查找完毕咯
    } while (true);
}

void CGUsbCamera::run()
{
    if(m_cameraName.isEmpty())
    {
        qDebug() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << "还未设置摄像头名称";
        return;
    }

#ifdef Q_OS_LINUX
    int ret = openCamera(QString("%1").arg(mCameName).toStdString());
#endif

#ifdef Q_OS_WIN32
    int ret = openCamera(QString("video=%1").arg(m_cameraName).toStdString());
#endif
    if(ret < 0)
    {
        qCritical() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << " open camera failed";
        return;
    }

    ret = initDecodec(m_pInputavFormatContext->streams[0]);
    if (ret < 0)
    {
        qCritical() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << " init decodec failed";
        return;
    }

    m_swsContext = sws_getContext(m_width, m_height, (AVPixelFormat)m_pInputavFormatContext->streams[0]->codecpar->format, m_width, m_height, AV_PIX_FMT_RGB32,
                                  SWS_BICUBIC, NULL, NULL, NULL);
    QCoreApplication::postEvent(m_eventReceiver, new InitRecordParametersEvent(), Qt::QueuedConnection); // 通知初始化录制参数

    // 用于图像采集，发送到界面
    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *swsVideoFrame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_decodec->width, m_decodec->height, 1);
    uint8_t *out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    // 根据指定的图像参数和提供的图像数据缓冲区设置图像域
    av_image_fill_arrays(swsVideoFrame->data, swsVideoFrame->linesize, out_buffer, AV_PIX_FMT_RGB32, m_decodec->width, m_decodec->height, 1);
    while(bCapture)
    {
        auto packet = readPacketFromSource(); // 使用智能指针
        if(packet && 0 == packet->stream_index)
        {
            if(true == decode(packet.get(), videoFrame))
            {
                // chw，采集图片
                // 分配和返回一个SwsContext你需要它来执行使用sws_scale()的缩放/转换操作
                // 在 pFrame->data 中缩放图像切片，并将得到的缩放切片放在pFrameRGB->data图像中
                sws_scale(m_swsContext, (const uint8_t *const *)videoFrame->data, videoFrame->linesize, 0, m_pInputavFormatContext->streams[0]->codecpar->height,
                        (uint8_t *const *)swsVideoFrame->data, swsVideoFrame->linesize);

                // 通过QByteArray传递图像，这样就不需要考虑图像指针的创建；通过postEvent发送，将抓帧和录制解耦
                QCoreApplication::postEvent(m_eventReceiver, new CaptureImageEvent(QByteArray().fromRawData((const char *)out_buffer, numBytes), m_decodec->width, m_decodec->height, QImage::Format_RGB32), Qt::QueuedConnection);
            }
        }

        if (nullptr != packet.get()) av_packet_unref(packet.get()); // 释放资源
    }

    qDebug() << "quit capture thread.";
    av_frame_free(&videoFrame);
    avcodec_close(m_decodec);
}

void CGUsbCamera::isCapture(bool is)
{
    bCaptureMutex.lock();
    bCapture = is;
    bCaptureMutex.unlock();
}

/**
 * @brief 执行ffmpeg命令行
 * @param cmd 命令
 * @return 返回值
 */
QString CGUsbCamera::ffmpegCMDexec(QString cmd)
{
    // 执行命令
    QProcess t_Process; // 应用程序类
    t_Process.setProcessChannelMode(QProcess::MergedChannels);
    t_Process.start("cmd"); // 启动cmd程序,传入参数
    bool isok = t_Process.waitForStarted();
    //    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<isok; // 打印启动是否成功

    QString command1 = "cd /d " + QCoreApplication::applicationDirPath() + "\r\n";
    t_Process.write(command1.toLatin1().data());
    t_Process.write(cmd.toLatin1().data());

    t_Process.closeWriteChannel(); // 关闭输入通道
    t_Process.waitForFinished();
    QString strTemp=QString::fromLocal8Bit(t_Process.readAllStandardOutput()); // 获取程序输出
    t_Process.close(); // 关闭程序

    return  strTemp;
}

/**
 * @brief 执行ffmpeg命令行,linux
 * @param cmd
 * @return
 */
QString CGUsbCamera::ffmpegCMDexecLinux(QString cmd)
{
    // 执行命令
    QProcess t_Process; // 应用程序类
    t_Process.setProcessChannelMode(QProcess::MergedChannels);
    t_Process.start("bash"); // 启动cmd程序, 传入参数
    bool isok = t_Process.waitForStarted();

    t_Process.write(cmd.toLatin1().data());

    t_Process.closeWriteChannel(); // 关闭输入通道
    t_Process.waitForFinished();
    QString strTemp = QString::fromLocal8Bit(t_Process.readAllStandardOutput()); // 获取程序输出
    t_Process.close(); // 关闭程序

    return  strTemp;
}

/**
 * @brief 获取计算机有效摄像头信息
 * @return
 */
QMap<QString, QString> CGUsbCamera::EnumVideoDevices()
{
    QMap<QString, QString> mapCameraDevs; // 计算机有效摄像头信息，<摄像头名,摄像头描述符>
#ifdef Q_OS_LINUX
    // linux系统
    QString ret = ffmpegCMDexecLinux("ls /dev/video*");
    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<ret;
    if(ret.contains("无法访问"))
        return mapCameraDevs;
    QStringList list = ret.split("\n");
    for(int index=0;index<list.size();index++)
    {
        if(list.at(index).isEmpty())
            continue;
        mapCameraDevs[list.at(index)] = list.at(index);
    }
    return mapCameraDevs;
#endif

#ifdef Q_OS_WIN32
    // windows系统
    QString ret = ffmpegCMDexec("ffmpeg.exe -list_devices true -f dshow -i dummy\r\n");
    if(!ret.contains("Alternative name"))
    {
        qDebug() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << "检测不到音视频设备";
        return mapCameraDevs;
    }

    lookupDevices(ret, mapCameraDevs, DT_Video);
    return mapCameraDevs;
#endif
}

/**
 * @brief 获取计算机音频设备信息
 * @return
 */
QMap<QString, QString> CGUsbCamera::EnumAudioDevices()
{
    QMap<QString, QString> mapAudioDevs; // 计算机有效音频信息，<音频名,音频描述符>
#ifdef Q_OS_LINUX
    // linux系统
#endif

#ifdef Q_OS_WIN32
    // windows系统
    QString ret = ffmpegCMDexec("ffmpeg.exe -list_devices true -f dshow -i dummy\r\n");
    if(!ret.contains("Alternative name"))
    {
        qDebug() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << "检测不到音视频设备";
        return mapAudioDevs;
    }

    lookupDevices(ret, mapAudioDevs, DT_Audio);
    return mapAudioDevs;
#endif
}

/**
 * @brief 获取摄像头分辨率
 * @param cameraName
 * @return  pixel_format#分辨率#帧率
 */
QStringList CGUsbCamera::EnumResolutions(QString cameraName)
{
    QStringList cameraResolutions;
#ifdef Q_OS_LINUX
    // 获取摄像头支持的分辨率
    // v4l2-ctl -d /dev/video0 --list-formats-ext//获取摄像头支持的图像格式、分辨率、帧率
    QString ret = ffmpegCMDexecLinux(QString("v4l2-ctl --list-framesizes=MJPG -d %1").arg(cameraName));
    cameraResolutions.clear();
    QStringList list = ret.split("\n");
    for(int index=0;index<list.size();index++)
    {
        if(list.at(index).contains("Discrete"))
        {
            QStringList listCame = list.at(index).split("Discrete");
            cameraResolutions.append(listCame.at(1).trimmed());
        }
    }

    return cameraResolutions;
#endif
#ifdef Q_OS_WIN32
    // windows系统
    QString cmdCommand = QString("ffmpeg.exe -list_options true -f dshow -i video=\"%1\"\r\n").arg(cameraName);
    QString ret = ffmpegCMDexec(cmdCommand);

    QRegExp regexp(R"(.*pixel_format=(.*)min.*s=(.*)fps=(.*)max.*\(.*\))");
    QStringList list = ret.split("\r\n");
    for (const QString &single : list)
    {
        if (false == regexp.exactMatch(single)) continue;

        QString format = regexp.cap(1).trimmed();
        QString resolution = regexp.cap(2).trimmed();
        QString fps = regexp.cap(3).trimmed();

        cameraResolutions.push_back(QString("%1(%2,%3)").arg(resolution).arg(format).arg(fps));
    }

    return cameraResolutions;
#endif
}

/**
 * @brief 转换编码，在Qt程序里显示中文
 * @param chars
 * @return
 */
QString CGUsbCamera::gbkToUnicode(const char *chars)
{
    QTextCodec *gbk= QTextCodec::codecForName("GBK");
    QString str = gbk->toUnicode(chars); // 获取的字符串可以显示中文

    return str;
}

/**
 * @brief Qt输出字符串到外部程序，在外部程序显示中文
 * @param str
 * @return
 */
QByteArray CGUsbCamera::gbkFromUnicode(QString str)
{
    QTextCodec *gbk = QTextCodec::codecForName("GBK");
    return gbk->fromUnicode(str);
}
