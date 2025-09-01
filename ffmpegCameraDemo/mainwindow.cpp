#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cgusbcamera.h"
#include "cgusbrecord.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->setScaledContents(true);

    qDebug() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << " " << avcodec_configuration();//打印ffmpeg配置
    qDebug() << "[" << __FILE__ << "]" << __LINE__ << __FUNCTION__ << " " << avcodec_version();//打印ffmpeg版本号

    m_cgusbCamera = new CGUsbCamera(this);
    m_cgusbCamera->isCapture(true);
    m_cgusbCamera->SetResolution(1280, 720, 30);

    m_cgusbRecord = new CGusbRecord();
    m_cgusbRecord->SetRecordFlag(true);
    m_cgusbRecord->SetResolution(1280, 720, 30);

    enumCameraDevInfo();
    connect(ui->comboBox_cameList, &QComboBox::currentTextChanged, this, &MainWindow::slotcomboBox_cameListChanged);
}

MainWindow::~MainWindow()
{
    if (nullptr != m_cgusbRecord)
    {
        delete m_cgusbRecord;
        m_cgusbRecord = nullptr;
    }

    m_cgusbCamera->isCapture(false);
    if(m_cgusbCamera->isRunning())
    {
        m_cgusbCamera->quit();
        m_cgusbCamera->wait();
    }

    if (nullptr != m_cgusbCamera)
    {
        delete m_cgusbCamera;
        m_cgusbCamera = nullptr;
    }
    delete ui;
}


void MainWindow::customEvent(QEvent *event)
{
    if (event->type() == InitRecordParametersEvent::type())
    {
        initRecordParametersEvent(static_cast<InitRecordParametersEvent *>(event));
    }
    else if (event->type() == CaptureImageEvent::type())
    {
        captureImageEvent(static_cast<CaptureImageEvent *>(event));
    }
    else
    {
        QMainWindow::customEvent(event);
    }
}


void MainWindow::printInputVideoDev()
{
    //打印视频音频设备
    AVFormatContext *pFmtCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_devices", "true", 0);
    const AVInputFormat *iformat = av_find_input_format("dshow");
    //printf("Device Info=============\n");
    avformat_open_input(&pFmtCtx, "video=dummy", iformat, &options);
    //printf("========================\n");
}

void MainWindow::enumCameraDevInfo()
{
    QMap<QString, QString> tmCameraDev = m_cgusbCamera->EnumVideoDevices();
    for(QMap<QString,QString>::iterator it = tmCameraDev.begin(); it != tmCameraDev.end(); it++)
    {
        ui->comboBox_cameList->addItem(it.key());
    }

    if(tmCameraDev.size() > 0)
    {
        slotcomboBox_cameListChanged(ui->comboBox_cameList->currentText());
    }
}

void MainWindow::initRecordParametersEvent(InitRecordParametersEvent *event)
{
    Q_UNUSED(event)
    do {
        int ret = m_cgusbRecord->InitEncodec();
        if (ret < 0)
        {
            qCritical() << "Record video init encodec failed: " << ret;
            break;
        }

        ret = m_cgusbRecord->OpenOutput();
        if (ret < 0)
        {
            qCritical() << "Record video open output failed: " << ret;
            break;
        }

        m_cgusbRecord->InitSwsContext();
        m_cgusbRecord->InitSwsFrame();
    } while(0);
}

void MainWindow::captureImageEvent(CaptureImageEvent *event)
{
    QImage image((uchar *)event->Image.data(), event->Width, event->Height, (QImage::Format)event->Format);
    ui->label->setPixmap(QPixmap::fromImage(image));

    m_cgusbRecord->WritePacket((uchar *)event->Image.data());
}

void MainWindow::slot_GetOneFrame(QImage image)
{
    ui->label->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::slotcomboBox_cameListChanged(QString text)
{
    QStringList list = m_cgusbCamera->EnumResolutions(text);
    ui->comboBox_rosul->clear();
    ui->comboBox_rosul->addItems(list);
}

//开始采集
void MainWindow::on_pushButton_start_clicked()
{
#ifdef Q_OS_LINUX
//linux系统
    QString paraStr = ui->comboBox_rosul->currentText();
    if(paraStr.isEmpty())
        return;
    QStringList list = paraStr.split("x");
    pCGUsbCamera->SetCameraResolution(list.at(0).toInt(),list.at(1).toInt(),30);
#endif

#ifdef Q_OS_WIN32
    if(false == ui->comboBox_rosul->currentText().isEmpty())
    {
        QRegExp regexp(R"((.*)x(.*)\((.*),(.*)\))");
        if (true == regexp.exactMatch(ui->comboBox_rosul->currentText()))
        {
            m_cgusbCamera->SetResolution(regexp.cap(1).toInt(), regexp.cap(2).toInt(), regexp.cap(4).toInt());
            m_cgusbRecord->SetResolution(regexp.cap(1).toInt(), regexp.cap(2).toInt(), regexp.cap(4).toInt());
        }
    }
#endif

    m_cgusbCamera->SetcameraName(ui->comboBox_cameList->currentText());
    m_cgusbCamera->start();
}
