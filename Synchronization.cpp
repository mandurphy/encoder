#include "Synchronization.h"
#include "Link.h"
#include "Config.h"
#include <Json.h>
#include <QFile>
#include "Channel.h"
#include "ChannelMix.h"
#include "ChannelVI.h"

Synchronization::Synchronization(QObject *parent) : QObject(parent)
{

}

void Synchronization::init()
{
    if(!QFile::exists("/link/config/auto/sync.json"))
    {
        QVariantMap iface=Json::loadFile("/link/config/board.json").toMap()["interfaceA"].toMap();
        QVariantList chnList=Json::loadFile("/link/config/config.json").toList();
        QVariantList list;
        QStringList aiList;
        QStringList aoList;
        for(int i=0;i<chnList.count();i++)
        {
            if(chnList[i].toMap()["type"].toString()!="vi")
                break;

            aiList<<chnList[i].toMap()["interface"].toString();
        }
        aiList<<"Line-In";
        aoList<<"Line-Out"<<"HDMI-OUT"<<"HDMI-OUT2"<<"SDI-OUT";


        for(int i=0;i<aiList.count();i++)
        {
            QString name=aiList[i];
            if(!iface.contains(name))
                continue;
            QVariantMap map;
            map["type"]="ai";
            map["name"]=name;
            map["defDelay"]=iface[name].toMap()["delay"].toInt();
            map["defDelay2"]=iface[name].toMap()["delay2"].toInt();
            map["delay"]=map["defDelay"].toInt();
            map["delay2"]=map["defDelay2"].toInt();
            list<<map;
        }


        for(int i=0;i<aoList.count();i++)
        {
            QString name=aoList[i];
            if(!iface.contains(name))
                continue;
            QVariantMap map;
            map["type"]="ao";
            map["name"]=name;
            map["defDelay"]=iface[name].toMap()["delay"].toInt();
            map["delay"]=map["defDelay"].toInt();
            list<<map;
        }

        Json::saveFile(list,"/link/config/auto/sync.json");
    }
    update(Json::loadFile("/link/config/auto/sync.json").toList());
}

bool Synchronization::update(QVariantList list)
{
    for(int i=0;i<list.count();i++)
    {
        QVariantMap map=list[i].toMap();
        if(map["type"].toString()=="ai")
        {
            LinkObject *ai;
            if(map["name"].toString()=="Line-In")
            {
                if(Channel::lineIn==NULL)
                    continue;
                ai=Channel::lineIn;
                if(Channel::alsa!=NULL)
                    ai=Channel::alsa;
            }
            else
            {
                ChannelVI *cv=(ChannelVI*)Config::findChannelById(i);
                ai=cv->audio;
                if(cv->alsa!=NULL)
                    ai=cv->alsa;
            }

            QVariantMap data;
            data["delay"]=map["delay"];
            data["delay2"]=map["delay2"];
            ai->setData(data);
        }
        else
        {
            LinkObject *ao;
            if(map["name"].toString()=="Line-Out")
                ao=Channel::lineOut;
            else if(map["name"].toString()=="HDMI-OUT")
                ao=((ChannelMix*)Config::chns.last())->outputA;
            else
                ao=((ChannelMix*)Config::chns.last())->outputA2;

            QVariantMap data;
            data["delay"]=map["delay"];
            ao->setData(data);
        }
    }
    Json::saveFile(list,"/link/config/auto/sync.json");
    return true;
}
