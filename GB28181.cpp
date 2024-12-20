#include "GB28181.h"
#include <QFile>
#include "Json.h"
#include "Config.h"

GB28181::GB28181(QObject *parent) : QObject(parent)
{

}

void GB28181::init()
{
    if(!QFile::exists("/link/config/auto/gb28181.json"))
    {
        QVariantMap map;
        QVariantMap server;
        server["Manufacture"]="Encoder";
        server["Mode"]="ENC";
        server["enable"]=false;
        server["id"]="34020000003000000001";
        server["passwd"]="12345678";
        server["realm"]="3402000000";
        server["svrId"]="34020000002000000001";
        server["svrIp"]="192.168.1.218";
        server["svrPort"]=5060;
        map["server"]=server;
        map["channel"]=QVariantList();
        Json::saveFile(map,"/link/config/auto/gb28181.json");
    }
    config=Json::loadFile("/link/config/auto/gb28181.json").toMap();

    QVariantList encCfg=Json::loadFile("/link/config/config.json").toList();
    if(config["channel"].toList().count()!=encCfg.count())
    {
        QVariantList list;
        for(int i=0;i<encCfg.count();i++)
        {
            QVariantMap map;
            map["id"]=encCfg[i].toMap()["id"].toInt();
            QString myid=config["server"].toMap()["id"].toString();
            myid=myid.left(myid.count()-5);
            myid=myid+QString::number(10000+map["id"].toInt());
            map["chnId"]=myid;
            map["enable"]=false;
            map["name"]=QString("Stream%1").arg(map["id"].toInt());
            list.append(map);
        }
        config["channel"]=list;
        Json::saveFile(config,"/link/config/auto/gb28181.json");
    }
    svr=Link::create("LinkGB28181");
    loadConfig(config);

}

bool GB28181::loadConfig(QVariantMap config)
{
    QVariantMap svrCfg=config["server"].toMap();

    if(svrCfg["enable"].toBool())
    {
        svrCfg["ip"]=Json::loadFile("/link/config/net.json").toMap()["ip"];
        svr->start(svrCfg);
        svr->invoke("regist");

        QVariantList list=config["channel"].toList();
        for(int i=0;i<list.count();i++)
        {
            QVariantMap cfg=list[i].toMap();
            int id=cfg["id"].toInt();
            QString chnId=cfg["chnId"].toString();
            if(!chnMap.contains(id))
                chnMap[id]=new GbChannel();
            chnMap[id]->cfg=cfg;
            if(cfg["enable"].toBool())
            {
                if(chnMap[id]->mux==NULL)
                    chnMap[id]->mux=Link::create("GBMux");
                chnMap[id]->mux->start();

                QVariantMap chnInfo;
                chnInfo["id"]=chnId;
                chnInfo["name"]=chnMap[id]->mux->name();
                QVariantMap map;
                map["DeviceID"]=chnId;
                map["Name"]=cfg["name"].toString();
                map["Manufacture"]="Encoder";
                map["Mode"]="Mode";
                map["Firmware"]="Firmware";
                map["Owner"]="Owner";
                map["CivilCode"]="CivilCode";
                map["Address"]="Address";
                map["Parental"]="0";
                map["Status"]="ON";
                map["SafetyWay"]=0;
                map["RegisterWay"]=1;
                map["Secrecy"]=0;

                chnInfo["info"]=map;
                svr->invoke("addChannel",chnInfo);

                Channel *chn=Config::findChannelById(id);
                chn->encV->linkV(chnMap[id]->mux)->linkV(svr);
                chn->encA->linkA(chnMap[id]->mux);

            }
            else
            {
                if(chnMap[id]->mux!=NULL)
                {
                    chnMap[id]->mux->stop();
                    svr->invoke("delChannel",chnId);
                }
            }

        }

    }

    return true;
}

bool GB28181::update(QString json)
{
    QVariantMap cfg=Json::decode(json).toMap();
       if(cfg.isEmpty())
           return false;


       if(!loadConfig(cfg))
           return false;

       config=cfg;

       QFile file("/link/config/auto/gb28181.json");
       file.open(QFile::ReadWrite);
       file.resize(0);
       file.write(json.toUtf8());
       file.close();

       return true;
}
