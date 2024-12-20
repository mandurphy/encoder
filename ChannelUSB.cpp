#include "ChannelUSB.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include "Json.h"

ChannelUSB::ChannelUSB(QObject *parent) : Channel(parent)
{
    alsa=Link::create("InputAlsa");
    audio=Link::create("Resample");
    video=Link::create("DecodeV");
    encA=Link::create("EncodeA");
    encV=Link::create("EncodeV");
    encV2=Link::create("EncodeV");
    usb=Link::create("InputV4l2");
}

void ChannelUSB::init(QVariantMap)
{

    usb->linkV(video);
    overlay->linkV(encV);
    overlay->linkV(encV2);


    alsa->linkA(audio);
    gain->linkA(encA);


    Channel::init();
    if(QFile::exists("/link/config/misc/preset.json"))
        presetList=Json::loadFile("/link/config/misc/preset.json").toList();
}

void ChannelUSB::updateConfig(QVariantMap cfg)
{

    if(cfg["enable"].toBool())
    {
        audio->start();
        video->start();
        if(cfg.contains("alsa"))
        {
            QVariantMap dataAlsa;
            dataAlsa["path"] = cfg["alsa"].toString();
            if(cfg.contains("lnk"))
                dataAlsa["lnk"] = cfg["lnk"].toString();
            if(cfg.contains("channels"))
                dataAlsa["channels"] = cfg["channels"].toInt();
            alsa->start(dataAlsa);
        }
        else
            alsa->start();

        {
            QVariantMap dd;
            dd["width"]=cfg["encv"].toMap()["width"].toInt();
            dd["height"]=cfg["encv"].toMap()["height"].toInt();
            dd["framerate"]=cfg["encv"].toMap()["framerate"].toInt();
            usb->start(dd);
        }

        if(cfg["encv"].toMap()["codec"].toString()!="close")
            encV->start(cfg["encv"].toMap());
        else
            encV->stop();

        if(cfg["encv2"].toMap()["codec"].toString()!="close")
            encV2->start(cfg["encv2"].toMap());
        else
            encV2->stop();

        if(cfg["enca"].toMap()["codec"].toString()!="close")
            encA->start(cfg["enca"].toMap());
        else
            encA->stop();
    }
    else
    {
        audio->stop();
        alsa->stop();
        usb->stop();
        encV->stop();
        encV2->stop();
    }

    Channel::updateConfig(cfg);
}

bool ChannelUSB::ptz_set(int p, int t, int z)
{
    QVariantList args;
    args<<p<<t<<z;
    usb->invoke("ptz_set",args);
    return true;
}

QVariantMap ChannelUSB::ptz_get()
{
    QVariantMap map;
    QVariantList list=usb->invoke("ptz_get").toList();
    if(list.count()!=3)
        return map;
    map["p"]=list[0];
    map["t"]=list[1];
    map["z"]=list[2];
    return map;
}

bool ChannelUSB::insta360_set(QVariantMap cfg)
{
    usb->invoke("insta360_set",cfg);
    return true;
}

QVariantMap ChannelUSB::insta360_get()
{
    return usb->invoke("insta360_get").toMap();
}

bool ChannelUSB::preset_set(int index, int p, int t, int z)
{
    QVariantList args;
    args<<p<<t<<z;
    presetList[index]=args;
    Json::saveFile(presetList,"/link/config/misc/preset.json");
    return true;
}

bool ChannelUSB::preset_call(int index)
{
    usb->invoke("ptz_set",presetList[index]);
    return true;
}
