#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cgusbcameracustomevent.h"

namespace Ui {
class MainWindow;
}

class CGUsbCamera;
class CGusbRecord;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();



protected:
    void customEvent(QEvent *event) override;

private:
    void printInputVideoDev(); //打印输入设备
    void enumCameraDevInfo(); //获取摄像头列表
    void initRecordParametersEvent(InitRecordParametersEvent *event);
    void captureImageEvent(CaptureImageEvent *event);

public slots:
    void slot_GetOneFrame(QImage image);
    void slotcomboBox_cameListChanged(QString text);

private slots:
    void on_pushButton_start_clicked();

private:
    Ui::MainWindow *ui;
    CGUsbCamera *m_cgusbCamera;
    CGusbRecord *m_cgusbRecord;
};

#endif // MAINWINDOW_H
