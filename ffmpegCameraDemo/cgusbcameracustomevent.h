#ifndef CGUSBCAMERACUSTOMEVENT_H
#define CGUSBCAMERACUSTOMEVENT_H

#include <QEvent>

enum E_CustomEventId {
    CustomEventId_InitRecordParameters = QEvent::User + 1,
    CustomEventId_CaptureImage,
};

class InitRecordParametersEvent : public QEvent
{
public:
    explicit InitRecordParametersEvent()
        : QEvent(type())
    { }

    static QEvent::Type type() { return static_cast<Type>(CustomEventId_InitRecordParameters); }
};

class CaptureImageEvent : public QEvent
{
public:
    explicit CaptureImageEvent(QByteArray image, int width, int height, int format)
        : QEvent(type()), Image(image), Width(width), Height(height), Format(format)
    { }

    static QEvent::Type type() { return static_cast<Type>(CustomEventId_CaptureImage); }
    const QByteArray Image;
    const int Width;
    const int Height;
    const int Format;
};


#endif // CGUSBCAMERACUSTOMEVENT_H
