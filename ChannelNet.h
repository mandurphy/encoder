#ifndef CHANNELNET_H
#define CHANNELNET_H

#include "Channel.h"

class ChannelNet : public Channel
{
    Q_OBJECT
public:
    explicit ChannelNet(QObject *parent = 0);
    virtual void init(QVariantMap);
    virtual void updateConfig(QVariantMap cfg);
    bool haveAudio;
    bool haveVideo;
    bool haveVideo2;
private:
    LinkObject *net;
    LinkObject *ndi;
    LinkObject *decA;
    LinkObject *ev;
    LinkObject *ea;
    LinkObject *queue;
    LinkObject *dv;
    QVariantMap streamInfo;
    bool containsB;
    void updateCodec(QVariantMap cfg);
signals:

public slots:
    void onNewEvent(QString type,QVariant info);
    void onNewEventDec(QString type,QVariant info);

};

#endif // CHANNELNET_H
