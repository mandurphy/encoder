#include "ChannelNDI.h"

ChannelNDI::ChannelNDI(QObject *parent) : Channel(parent)
{
    video=Link::create("DecodeV");
    ndi=Link::create("NDIRecv");
    audio=Link::create("Resample");
    decA=Link::create("DecodeA");
    encV=ndi;
    encA=ndi;
    encV2=NULL;
}

void ChannelNDI::init(QVariantMap)
{

    ndi->linkV(video);
    ndi->linkA(decA)->linkA(audio);
    Channel::init();
}

void ChannelNDI::updateConfig(QVariantMap cfg)
{
    if(cfg["enable"].toBool())
    {
        decA->start();
        audio->start();
        ndi->start();
        video->start();
        if(curName!=cfg["ndirecv"].toMap()["name"].toString())
        {
            snap->invoke("reset");
            curName=cfg["ndirecv"].toMap()["name"].toString();
        }
        ndi->setData(cfg["ndirecv"].toMap());
    }
    else
    {
        QVariantMap dd;
        dd["name"]="";
        curName="";
        ndi->setData(dd);
        ndi->stop();
        video->stop();
    }
    Channel::updateConfig(cfg);
}

