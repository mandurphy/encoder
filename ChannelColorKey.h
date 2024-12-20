#ifndef CHANNELCOLORKEY_H
#define CHANNELCOLORKEY_H

#include "Channel.h"

class ChannelColorKey : public Channel
{
    Q_OBJECT
public:
    explicit ChannelColorKey(QObject *parent = nullptr);
    virtual void init(QVariantMap);
    virtual void updateConfig(QVariantMap cfg);
    LinkObject *colorKey;
    LinkObject *image;
    LinkObject *lastSrcB;

signals:

public slots:
};

#endif // CHANNELCOLORKEY_H
