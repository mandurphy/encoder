#ifndef LED_H
#define LED_H

#include <QObject>
#include <QVariantMap>
#include <QRgb>
#include <QTimer>
#include "Link.h"
#define LEDCFGFILE "/link/config/led/config.json"
#define LEDDEVFILE "/link/config/led/dev.json"

class LED : public QObject
{
    Q_OBJECT
public:
    explicit LED(QObject *parent = nullptr);
    void init();
    enum Mode{
        ModeCut,ModeBreathe,ModeFlick,ModeSlide
    }mode;

private:
    QVariantMap config;
    QVariantMap devCfg;
    QVariant curInfo;
    QString func;
    QVariantMap funcArgs;
    int fd;
    QTimer timer;
    uchar dstR,dstG,dstB;
    uchar lastR,lastG,lastB;
    LinkObject *tallyArbiter;
signals:

public slots:
    bool update(QVariantMap cfg);
    void onNewEvent(QString type,QVariant info);
    void onNewInfo();
    void setLED(int state);
    void fillColor(uchar r, uchar g, uchar b);
    void onTimer();

    QVariantList getTADevices();
};

extern LED *GLED;
#endif // LED_H
