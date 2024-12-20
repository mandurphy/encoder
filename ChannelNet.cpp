#include "ChannelNet.h"
#include <QFile>
#include <unistd.h>
#include "Json.h"
#include "Config.h"

ChannelNet::ChannelNet(QObject *parent) :
    Channel(parent)
{
    dv=Link::create("DecodeV");
    net=Link::create("InputNet");
#if !defined HI3516E && !defined HI3516CV610
    ndi=Link::create("NDIRecv");
#else
    ndi=NULL;
#endif
    audio=Link::create("Resample");
    decA=Link::create("DecodeA");
    ev=Link::create("EncodeV");
    encV2=Link::create("EncodeV");
    ea=Link::create("EncodeA");
    queue=Link::create("Queue");
    video=Link::create("Crop");
    encV=queue;
    encA=queue;

    haveAudio=false;
    haveVideo=false;
    haveVideo2=false;
    containsB = false;
}

void ChannelNet::init(QVariantMap)
{
    dv->linkV(video);
    video->linkV(ev);
    video->linkV(encV2);
    gain->linkA(ea);

    queue->linkV(dv);
    queue->linkA(decA);
    net->linkV(dv);
    net->linkA(decA)->linkA(audio);
    dv->linkE(decA);

    bool plate = false;
    if(QFile::exists(HARDWAREPATH))
    {
        QVariantMap hardware = Json::loadFile(HARDWAREPATH).toMap();
        plate = hardware["function"].toMap()["netPic"].toBool();
    }
    if(plate)
    {
        QVariantMap dd;
        dd["pic"] = "/link/config/nosignal.yuv";
        dv->setData(dd);
    }

    connect(net,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
    if(ndi != NULL)
        connect(ndi,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
    connect(dv,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEventDec(QString,QVariant)));

    Channel::init();
}

void ChannelNet::updateConfig(QVariantMap cfg)
{
    if(cfg["enable"].toBool() )
    {
        QVariantMap nd;
        QString path = cfg["net"].toMap()["path"].toString();
        nd["path"]=path;
        nd["protocol"]=cfg["net"].toMap()["protocol"].toString();
        nd["minDelay"]=cfg["net"].toMap()["minDelay"].toInt();
        nd["timeout"]=20;

        int bm=cfg["net"].toMap()["bufferMode"].toInt();

        if(bm==0)
        {
            nd["lowLatency"]=false;
            nd["buffer"]=true;
            nd["sync"]=false;
            nd["timestampMode"]="auto";
        }
        else if(bm==1)
        {
            nd["lowLatency"]=true;
            nd["buffer"]=false;
            nd["sync"]=false;
            nd["timestampMode"]="auto";
        }
        else if(bm==2)
        {
            nd["lowLatency"]=true;
            nd["buffer"]=false;
            nd["sync"]=false;
            nd["timestampMode"]="auto";
        }
        else if(bm==3)
        {
            nd["lowLatency"]=true;
            nd["buffer"]=false;
            nd["sync"]=false;
            nd["timestampMode"]="sync";
        }

        if(containsB || bm==2 || bm==3)
        {
            QVariantMap dd;
            dd["delay"]=cfg["net"].toMap()["minDelay"].toInt();
            queue->start(dd);
        }
        else
            queue->stop();

        if(ndi != NULL && (path.toLower().startsWith("ndi://")))
        {
            net->unLinkV(queue);
            net->unLinkA(queue);
            net->unLinkV(dv);
            net->unLinkA(decA);
            net->stop();

            if(bm == 2 || bm ==3)
            {
                ndi->unLinkV(dv);
                ndi->unLinkA(decA);
                ndi->linkV(queue);
                ndi->linkA(queue);
            }
            else
            {
                ndi->unLinkV(queue);
                ndi->unLinkA(queue);
                ndi->linkV(dv);
                ndi->linkA(decA);
            }
            QVariantMap ndd;
            ndd["name"] = path.replace("ndi://","");
            ndd["name"] = path.replace("NDI://","");
            ndi->start(ndd);
        }
        else
        {
            if(ndi != NULL)
            {
                ndi->unLinkV(queue);
                ndi->unLinkA(queue);
                ndi->unLinkV(dv);
                ndi->unLinkA(decA);
                ndi->stop();
            }
            if(containsB || bm == 2 || bm ==3)
            {
                net->unLinkV(dv);
                net->unLinkA(decA);
                net->linkV(queue);
                net->linkA(queue);
            }
            else
            {
                net->unLinkV(queue);
                net->unLinkA(queue);
                net->linkV(dv);
                net->linkA(decA);
            }
            net->start(nd);
        }


        LinkObject *nextEncV=encV;
        if(cfg["net"].toMap()["decodeV"].toBool())
        {
            QVariantMap cropMap;
            if(cfg.contains("cap"))
            {
                cropMap = cfg["cap"].toMap()["crop"].toMap();
                cropMap["pass"] = true;
            }
            video->start(cropMap);

            if(cfg["encv"].toMap()["codec"].toString()!="close")
            {
                nextEncV=ev;
                QVariantMap frm;
                frm["srcFramerate"]=cfg["net"].toMap()["framerate"].toInt();
                ev->setData(frm);
                encV2->setData(frm);

                QVariantMap encvMap = cfg["encv"].toMap();
                ev->start(encvMap);
            }
            else
            {
                nextEncV=net;
                if(containsB || bm==2 || bm==3)
                    nextEncV=queue;
                ev->stop();
            }

            if(cfg["enable2"].toBool() && cfg["encv2"].toMap()["codec"].toString()!="close")
            {
                QVariantMap encv2Map = cfg["encv2"].toMap();
                encV2->start(encv2Map);
            }
            else
            {
                encV2->stop();
            }
            QVariantMap decData;
            if(cfg.contains("cap"))
                decData["rotate"] = cfg["cap"].toMap()["rotate"].toInt();

            dv->start(decData);
        }
        else
        {
            dv->stop();
            ev->stop();
            video->stop();
            nextEncV=net;
            if(containsB || bm==2 || bm==3)
                nextEncV=queue;
        }

        if(nextEncV!=encV)
        {
            foreach(QString key,streamMap.keys())
            {
                encV->unLinkV(streamMap[key]->mux);
                streamMap[key]->mux->stop(key!="srt");
                nextEncV->linkV(streamMap[key]->mux);
            }
            encV=nextEncV;
            qDebug()<<encV->name();
        }


        LinkObject *nextEncA=encA;
        if(cfg["net"].toMap()["decodeA"].toBool())
        {
            decA->start();
            audio->start();
            nextEncA=ea;

            if(cfg["enca"].toMap()["codec"].toString()=="close")
                ea->stop();
            else
                ea->start(cfg["enca"].toMap());
        }
        else
        {
            decA->stop();
            audio->stop();
            nextEncA=net;

            if(containsB || bm==2 || bm==3)
                nextEncA=queue;
        }

        if(nextEncA!=encA)
        {
            foreach(QString key,streamMap.keys())
            {
                encA->unLinkA(streamMap[key]->mux);
                streamMap[key]->mux->stop(key!="srt");
                nextEncA->linkA(streamMap[key]->mux);

                if(streamMap_sub.contains(key))
                {
                    encA->unLinkA(streamMap_sub[key]->mux);
                    streamMap_sub[key]->mux->stop(key!="srt");
                    nextEncA->linkA(streamMap_sub[key]->mux);
                }
            }
            encA=nextEncA;
        }

    }
    else
    {
        ev->stop();
        dv->stop();
        net->stop();
        if(ndi != NULL)
            ndi->stop();
        encV2->stop();
        decA->stop();
        ea->stop();
        queue->stop();
        audio->stop();
    }
    containsB = false;
    updateCodec(cfg);
    Channel::updateConfig(cfg);
}

void ChannelNet::updateCodec(QVariantMap cfg)
{
    haveVideo=streamInfo["infoV"].toMap().contains("codec");
    haveVideo2=haveVideo;
    haveAudio=streamInfo["infoA"].toMap().contains("codec");

    if(haveVideo && cfg["net"].toMap()["decodeV"].toBool() && cfg["encv"].toMap()["codec"].toString()=="close")
        haveVideo=false;
    if(haveVideo && cfg["net"].toMap()["decodeV"].toBool() && cfg["encv2"].toMap()["codec"].toString()=="close")
        haveVideo2=false;
    if(haveAudio && cfg["net"].toMap()["decodeA"].toBool() && cfg["enca"].toMap()["codec"].toString()=="close")
        haveAudio=false;
}

void ChannelNet::onNewEvent(QString type, QVariant info)
{
    if(type=="ready")
    {
        streamInfo=info.toMap();
        updateCodec(data);
        Channel::updateConfig(data);
    }
}

void ChannelNet::onNewEventDec(QString type, QVariant info)
{
    if(type == "containsB")
    {
        bool hadB = info.toBool();
        if(hadB)
        {
            containsB = hadB;
            updateConfig(data);
        }
    }
}
