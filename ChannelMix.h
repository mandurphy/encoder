#ifndef CHANNELMIX_H
#define CHANNELMIX_H

#include "Channel.h"

class ChannelMix : public Channel
{
    Q_OBJECT
public:
    explicit ChannelMix(QObject *parent = 0);
    virtual void init(QVariantMap cfg);
    virtual void updateConfig(QVariantMap cfg);

    QList<int> curAList;
    LinkObject *outputV;
    LinkObject *outputV2;
    LinkObject *outputA;
    LinkObject *outputA2;

    LinkObject *lastSrcV;
    LinkObject *lastSrcV2;
    LinkObject *lastSrcA;
    LinkObject *lastSrcA2;
    LinkObject *lastSrcALine;
    LinkObject *lineOutGain;

    bool hadOutputLine;
signals:

public slots:
};

#endif // CHANNELMIX_H
