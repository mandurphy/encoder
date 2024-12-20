#include "ChannelColorKey.h"
#include "Config.h"
#include <QVariantMap>

ChannelColorKey::ChannelColorKey(QObject *parent) : Channel(parent)
{
    colorKey=Link::create("ColorKey");
    encA=Link::create("EncodeA");
    encV=Link::create("EncodeV");
    encV2=Link::create("EncodeV");
    image=Link::create("InputImage");
    video=colorKey;
    audio=NULL;
    lastSrcB=NULL;
}

void ChannelColorKey::init(QVariantMap)
{
    overlay->linkV(encV);
    overlay->linkV(encV2);
    gain->linkA(encA);
    Channel::init();
}

void ChannelColorKey::updateConfig(QVariantMap cfg)
{
    if(cfg["enable"].toBool())
    {
        QVariantMap colorData=cfg["colorKey"].toMap();

        Channel *chnA=Config::findChannelById(colorData["srcA"].toInt());
        if(chnA==NULL)
            return;
        LinkObject *srcA=chnA->audio;
        colorData["srcA"]=srcA->name();


        LinkObject *srcB;
        QString type=colorData["srcB"].toMap()["type"].toString();
        if(type=="img")
        {
            QVariantMap imgData;
            imgData["path"]=colorData["srcB"].toMap()["path"];
            imgData["framerate"]=30;
            image->start(imgData);
            srcB=image;
        }
        else
        {
            image->stop();
            Channel *chnB=Config::findChannelById(colorData["srcB"].toMap()["id"].toInt());
            if(chnB==NULL)
                return;

            srcB=chnB->overlay;
        }

        colorData["srcB"]=srcB->name();
        colorKey->start(colorData);
        srcA->linkV(colorKey);
        if(lastSrcB!=NULL)
            lastSrcB->unLinkV(colorKey);
        srcA->linkV(colorKey);
        srcB->linkV(colorKey);
        lastSrcB=srcB;

        if(cfg["enca"].toMap()["codec"].toString()!="close")
            encA->start(cfg["enca"].toMap());
        else
            encA->stop();

        if(cfg["encv"].toMap()["codec"].toString()!="close")
            encV->start(cfg["encv"].toMap());
        else
            encV->stop();

        if(cfg["enable2"].toBool()  && cfg["encv2"].toMap()["codec"].toString()!="close")
            encV2->start(cfg["encv2"].toMap());
        else
            encV2->stop();
    }
    else
    {
        colorKey->stop();
        image->stop();
        encV->stop();
        encV2->stop();
        encA->stop();
    }


    Channel::updateConfig(cfg);
}

