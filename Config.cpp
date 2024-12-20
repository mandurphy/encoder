#include "Config.h"
#include "Json.h"
#include <QVariantList>
#include <QVariantMap>
#include "ChannelVI.h"
#include "ChannelNet.h"
#include "ChannelMix.h"
#include "ChannelUSB.h"
#include "ChannelFile.h"
#include "ChannelNDI.h"
#include "ChannelColorKey.h"

QList<Channel*> Config::chns;
QVariantList Config::overlayList;
QVariantList Config::roiList;
Config::Config(QObject *parent) :
    QObject(parent)
{
}

void Config::loadConfig(QString path)
{
    QVariantList list=Json::loadFile(path).toList();
    if(list.count()==0)
    {
        QString cmd;
        cmd="cp "+path+".bak "+path;
        system(cmd.toLatin1().data());
        list=Json::loadFile(path).toList();
    }

    bool firstLoad=chns.isEmpty();
    for(int i=0;i<list.count();i++)
    {
        QVariantMap cfg=list[i].toMap();
        int id=cfg["id"].toInt();
        QString chnName = cfg["name"].toString();
        QString type=cfg["type"].toString();
        Channel *chn=findChannelById(id);
        if(chn==NULL)
        {
            if(type=="vi")
            {
                chn=new ChannelVI();
                if(cfg.contains("avs"))
                {
                    chn->enableAVS=cfg["avs"].toBool();
                }
            }
            else if(type=="net")
                chn=new ChannelNet();
            else if(type=="mix")
                chn=new ChannelMix();
            else if(type=="usb")
                chn=new ChannelUSB();
            else if(type=="file")
                chn=new ChannelFile();
            else if(type=="ndi")
                chn=new ChannelNDI();
            else if(type=="colorKey")
                chn=new ChannelColorKey();
            chn->type=type;
            chn->id=id;
            chn->chnName = chnName;
            if(firstLoad)
                chn->init(cfg);
            chns.append(chn);
        }
        if(cfg!=chn->data && !firstLoad)
            chn->updateConfig(cfg);
    }

    if(firstLoad)
    {
        for(int i=0;i<chns.count();i++)
        {
            QVariantMap cfg=list[i].toMap();
            chns[i]->updateConfig(cfg);
        }
    }

    QString cmd;
    cmd="cp "+path+" "+path+".bak";
    system(cmd.toLatin1().data());
}

Channel *Config::findChannelById(int id)
{
    for(int i=0;i<chns.count();i++)
    {
        if(chns[i]->id==id)
            return chns[i];
    }
    return NULL;
}

void Config::loadAutoConfig()
{
    int chnCount=chns.count();
    roiList=Json::loadFile(ROIPATH).toList();
    if(roiList.count()!=chnCount)
    {
        QVariantList list;
        QVariantMap map;
        map["enable"]=false;
        map["abs"]=false;
        map["framerate"]=-1;
        map["x"]=0.3;
        map["y"]=0.3;
        map["w"]=0.3;
        map["h"]=0.3;
        for(int i=0;i<8;i++)
            list<<map;

        roiList.clear();
        for(int i=0;i<chnCount;i++)
            roiList<<(QVariant)list;
        Json::saveFile(roiList,ROIPATH);
    }

    overlayList=Json::loadFile(OVERLAYPATH).toList();
    if(overlayList.count()!=chnCount)
    {
        overlayList.clear();
        QVariant overLayDemo = Json::loadFile("/link/config/misc/overlayDemo.json");
        for(int i=0;i<chnCount;i++)
            overlayList << overLayDemo;
        Json::saveFile(overlayList,OVERLAYPATH);
    }

    for(int i=0;i<chns.count();i++)
    {
        Channel *chn=chns[i];
        if(chn->enable)
        {
            QVariantMap lays;
            lays["lays"]=overlayList[i].toList();
            chn->overlay->start(lays);

            if(chn->encV!=NULL && chn->encV->name().startsWith("EncodeV"))
            {
                QVariantMap roi;
                roi["roi"]=roiList[i];
                chn->encV->setData(roi);
            }
        }
    }
}

