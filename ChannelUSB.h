#ifndef CHANNELUSB_H
#define CHANNELUSB_H

#include "Channel.h"

class ChannelUSB : public Channel
{
    Q_OBJECT
public:
    explicit ChannelUSB(QObject *parent = 0);
    virtual void init(QVariantMap);
    virtual void updateConfig(QVariantMap cfg);
private:
    LinkObject *usb;
    LinkObject *alsa;
    QVariantList presetList;
signals:

public slots:
    bool ptz_set(int p, int t, int z);
    QVariantMap ptz_get();
    bool insta360_set(QVariantMap cfg);
    QVariantMap insta360_get();
    bool preset_set(int index,int p,int t,int z);
    bool preset_call(int index);
};

#endif // CHANNELUSB_H
