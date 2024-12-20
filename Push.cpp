#include "Push.h"
#include "Config.h"
#include <QFile>
#include "Json.h"

Push::Push(QObject *parent) : QObject(parent)
{
    srcA=NULL;
    srcV=NULL;
    lastSrcA=NULL;
    lastSrcV=NULL;
    bPushing=false;
    pushCount=0;
    preview=Link::create("Mux");
    QVariantMap data;
    data["path"]="rtmp://127.0.0.1/live/preview";
    preview->setData(data);

    pushTimer = new QTimer(this);
    connect(pushTimer,SIGNAL(timeout()),this,SLOT(onPushCountTimer()));

    QVariantMap webVer = Json::loadFile("/link/config/misc/webVer.json").toMap();
    web = webVer["web"].toString();

}

void Push::init()
{
    QFile file("/link/config/push.json");
    file.open(QFile::ReadOnly);
    QString json=file.readAll();
    file.close();
    update(json);
    if(config["autorun"].toBool())
        start();

}

bool Push::start()
{
    foreach(PushUrl *url,urlList)
    {
        if(url->enable)
            url->mux->start();
    }
    preview->start();
    bPushing=true;
    pushTimer->start(1000);
    emit this->newEvent("push",true);
    return true;
}

bool Push::stop()
{
    pushTimer->stop();
    pushCount=0;
    bPushing=false;
    foreach(PushUrl *url,urlList)
    {
        url->count = 0;
        url->mux->stop();
    }
    preview->stop();
    emit this->newEvent("push",false);

    return true;
}

QVariantMap Push::getState()
{
    QVariantMap ret;
    ret["duration"]=pushCount;
    ret["pushing"]=bPushing;
    QVariantList speedList;
    foreach(PushUrl *url,urlList)
    {
        if(bPushing && url->enable)
            speedList<<(url->mux->invoke("getSpeed").toMap()["speed"].toInt()*8/1024);
        else
            speedList<<0;
    }
    ret["speed"]=speedList;
    QVariantList statusList;
    for(int i=0;i<urlList.count();i++)
    {
        PushUrl *url = urlList[i];
        QVariantMap durMap;
        durMap["type"] = url->type;
        durMap["duration"] = url->count;
        durMap["path"] = url->path;
        durMap["speed"] = speedList[i];
        statusList << durMap;
    }
    ret["status"]=statusList;
    return ret;
}

bool Push::update(QString json)
{
    config=Json::decode(json).toMap();

    Channel *chn=Config::findChannelById(config["srcA"].toInt());
    if(chn!=NULL)
        srcA=chn->encA;

    QString srcV_chn = config["srcV_chn"].toString();
    if(srcV_chn.isEmpty() || srcV_chn.isNull())
        srcV_chn = "main";

    chn=Config::findChannelById(config["srcV"].toInt());
    if(chn!=NULL)
    {
        if(srcV_chn == "main")
            srcV=chn->encV;
        else
            srcV=chn->encV2;
    }

    if(srcV==NULL)
        return false;


    QVariantMap preData = {{"mute",false}};
    if(srcA==NULL)
        preData["mute"] = true;

    preview->setData(preData);

    QVariantList list=config["url"].toList();

    for(int i=0;i<list.count() || i<urlList.count();i++)
    {
        PushUrl *tmp;
        if(i>=urlList.count())
        {
            tmp = new PushUrl();
            tmp->mux = Link::create(list[i].toMap()["type"].toString() != "webrtc" ? "Mux"
                : (
            #if !defined(HI3521D) && !defined(HI3531D)
                "WebRTCPush"
            #else
                "Mux"
            #endif
            ));
            urlList.append(tmp);
        }

        if(i>=list.count())
        {
            urlList.last()->mux->stop(true);
            delete urlList.last()->mux;
            urlList.removeLast();
            continue;
        }

        QVariantMap item = list[i].toMap();
        tmp=urlList[i];
        tmp->enable=item["enable"].toBool();
        tmp->path=item["path"].toString();
        tmp->type = "";
        if(item.contains("type"))
            tmp->type = item["type"].toString();
        tmp->flvflags = "";
        if(item.contains("flvflags"))
            tmp->flvflags=item["flvflags"].toString();
        if(item.contains("push"))
            tmp->push = item["push"].toBool();

        QVariantMap data;
        data["path"]=tmp->path;
        data["bufLenV"]=256;
        data["bufLenA"]=1024;
        data["thread"]=true;
        data["flvflags"] = tmp->flvflags;
        if(tmp->type == "webrtc")
            data["bearer"] = "";

        if(srcA==NULL)
            data["mute"]=true;
        else
        {
            if(lastSrcA!=NULL && lastSrcA!=srcA)
            {
                lastSrcA->unLinkA(tmp->mux);
                lastSrcA->unLinkA(preview);
            }
            srcA->linkA(tmp->mux);
            srcA->linkA(preview);
        }

        if(lastSrcV!=NULL && lastSrcV!=srcV)
        {
            lastSrcV->unLinkV(tmp->mux);
            lastSrcV->unLinkV(preview);
        }
        srcV->linkV(tmp->mux);
        srcV->linkV(preview);

        tmp->mux->setData(data);

        if(bPushing)
        {
            if(tmp->enable)
                tmp->mux->start();
            else
                tmp->mux->stop();
        }
    }

    lastSrcA=srcA;
    lastSrcV=srcV;

    Json::saveFile(config,"/link/config/push.json");

    return true;
}

void Push::onPushCountTimer()
{
    pushCount+=1000;
    foreach(PushUrl *url,urlList)
    {
        if(bPushing && url->enable)
            url->count += 1000;
        else
            url->count = 0;
    }
}


