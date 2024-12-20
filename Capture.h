#ifndef CAPTURE_H
#define CAPTURE_H

#include <QObject>
#include "Link.h"

#define CAPTUREFILE "/link/config/misc/capture.json"

class Capture : public QObject
{
    Q_OBJECT
public:
    explicit Capture(QObject *parent = nullptr);
    void init();
private:
    QVariantMap config;
    LinkObject *srcV;
    LinkObject *srcA;
    LinkObject *enc;
    LinkObject *res;
    LinkObject *uvc;
    LinkObject *uac;
signals:

public slots:
    bool update(QVariantMap cfg);
    void onNewEvent(QString type,QVariant info);
};

extern Capture *GCapture;
#endif // CAPTURE_H
