#include "RPC.h"
#include "Config.h"
#include "Json.h"
#include "ChannelFile.h"
#include <QFile>
#include <QDateTime>
#include <ChannelVI.h>
#include "Record.h"
#include "Push.h"
#include "UART.h"
#include "Intercom.h"
#include <QProcess>
#include <QDate>
#include "ChannelUSB.h"
#include "GB28181.h"
#include "Synchronization.h"
#include "oled/Oled.h"
#include"ChannelMix.h"
#include "LED.h"
#include "Capture.h"

RPC::RPC(QObject *parent) :
    QObject(parent),timerSyncRTC(this)
{
    device=Link::create("Device");
    device->start();
}

void RPC::startNTP()
{
//    QVariantMap ntp=Json::loadFile("/link/config/ntp.json").toMap();
//    if(ntp["enable"].toBool())
//    {
        onTimerSyncRTC();
        connect(&timerSyncRTC,SIGNAL(timeout()),this,SLOT(onTimerSyncRTC()));
        timerSyncRTC.start(30000);
//    }
}

void RPC::init()
{
    group=new Group(this);
    group->init();
    rpcServer=new jcon::JsonRpcTcpServer();
    jcon::JsonRpcServer::ServiceMap map;
    map[this]="enc";
    map[group]="group";
    map[GRecord]="rec";
    map[GPush]="push";
    map[GUart]="uart";
    map[GIntercom]="intercom";
    map[Ggb28181]="gb28181";
    map[GSync]="sync";
    map[GOLED]="oled";
    map[GLED]="led";
    map[GCapture]="capture";
    foreach(Channel *chn,Config::chns)
    {
        if(chn->type=="usb")
        {
            ChannelUSB *usb=(ChannelUSB*)chn;
            map[usb]="usb";
        }
    }
    rpcServer->registerServices(map, ".");
    rpcServer->listen(6001);

#ifndef  HI3516E
    if(Config::chns[0]->streamMap.contains("ndi"))
            Config::chns[0]->streamMap["ndi"]->mux->linkE(device);

#endif
}


bool RPC::update(QString json)
{
    if(CFGPATH != "/link/config/config.json")
        return false;

    QFile file(CFGPATH);
    if(!file.open(QFile::ReadWrite))
        return false;
    file.resize(0);
    file.write(json.toUtf8());
    file.close();
    Config::loadConfig(CFGPATH);
    return true;
}

bool RPC::updateOverlay(QString json)
{
    QFile file(OVERLAYPATH);
    if(!file.open(QFile::ReadWrite))
        return false;
    file.resize(0);
    file.write(json.toUtf8());
    file.close();
    Config::loadAutoConfig();
    return true;
}

bool RPC::updateRoi(QString json)
{
    QFile file(ROIPATH);
    if(!file.open(QFile::ReadWrite))
        return false;
    file.resize(0);
    file.write(json.toUtf8());
    file.close();
    Config::loadAutoConfig();
    return true;
}


QVariantList RPC::snap(const QString &json)
{
    QVariantList retList;
    static qint64 lastPts=0;
    qint64 now=QDateTime::currentDateTime().toMSecsSinceEpoch();
    if(now-lastPts<400)
        return retList;

    int mod=0;
    QVariantList chnIds;
    if(!json.isEmpty())
    {
        QVariantMap params = Json::decode(json).toMap();
        if(params.contains("mod"))
            mod = params["mod"].toInt();
        if(params.contains("chns"))
            chnIds = params["chns"].toList();
    }

    for(int i=0;i<Config::chns.count();i++)
    {
        Channel* chn = Config::chns[i];
        if(!chn->enable)
            continue;

        if(chnIds.isEmpty() || chnIds.contains(chn->id))
        {
            QVariantMap mp;
            mp["chn"+QString::number(chn->id)] = Config::chns[i]->doSnap(mod,chnIds);
            retList << mp;
        }
    }
    return retList;
}

QVariantMap RPC::getSysState()
{
    static qlonglong last_total=0;
    static qlonglong last_idel=0;
    static int last_cpu=0;


    qlonglong total=0,idel=0;
    QFile file("/proc/stat");
    file.open(QFile::ReadOnly);
    QString str=file.readLine();
    str=str.mid(5);
    file.close();
    QStringList lst=str.split(" ",QString::SkipEmptyParts);
    lst.setSharable(false);

    for(int i=0;i<lst.count();i++)
    {
        total+=lst.at(i).toLongLong();
        if(i==3)
            idel=lst.at(i).toLongLong();
    }
    int cpu=0;
    if(total-last_total!=0 && last_total!=0)
        cpu=100-(idel-last_idel)*100/(total-last_total);

    if(total-last_total>300)
    {
        last_total=total;
        last_idel=idel;
        last_cpu=cpu;
    }
    else
        cpu=last_cpu;

    QFile file2("/proc/meminfo");
    file2.open(QFile::ReadOnly);
    QString str1=file2.readLine();
    file2.readLine();
    QString str2=file2.readLine();
    //    QString str3=file2.readLine();
    file2.close();

    QRegExp rx("(\\d+)");
    rx.indexIn(str1);
    long mem1=rx.cap(1).toLong();
    rx.indexIn(str2);
    long mem2=rx.cap(1).toLong();
    //    rx.indexIn(str3);
    //    long mem3=rx.cap(1).toLong();

    int mem=100-(mem2)*100/mem1;


    QVariantMap rt;
    rt["cpu"]=cpu;
    rt["mem"]=mem;
    rt["temperature"]=device->invoke("getTemperature").toInt();
    return rt;
}

QVariantList RPC::getInputState()
{
    QVariantList list;
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="vi")
        {
            ChannelVI *vi=(ChannelVI*)Config::chns[i];
            list.append(vi->vi->invoke("getReport").toMap());
        }
    }
    return list;
}

QVariantMap RPC::getNetState()
{
    static qlonglong lastRx=0;
    static qlonglong lastTx=0;
    static qint64 lastPTS=0;

    qint64 now=QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 span=now-lastPTS;
    lastPTS=now;
    QFile file("/proc/net/dev");
    file.open(QFile::ReadOnly);

    qlonglong rx=0,tx=0;
    for(;;)
    {
        QString line = file.readLine();
        if(line.isEmpty())
            break;

        if(line.contains("wlan0"))
        {
            rx=line.split(' ',QString::SkipEmptyParts).at(1).toLongLong()*8;
            tx=line.split(' ',QString::SkipEmptyParts).at(9).toLongLong()*8;
        }
        else
        {
            if(line.contains("eth0") || line.contains("eth1"))
            {
                rx=line.split(' ',QString::SkipEmptyParts).at(1).toLongLong()*8;
                tx=line.split(' ',QString::SkipEmptyParts).at(9).toLongLong()*8;
            }
        }

        if(rx > 0 || tx > 0)
            break;

    }
    file.close();

    int speedrx=(rx-lastRx)*1000/span/1024;
    int speedtx=(tx-lastTx)*1000/span/1024;
    lastRx=rx;
    lastTx=tx;

    QVariantMap rt;
    rt["rx"]=speedrx;
    rt["tx"]=speedtx;

    return rt;
}

QVariantList RPC::getEPG()
{
    QVariantList chnList=Json::loadFile(CFGPATH).toList();
    QVariantList ret;
    QVariantMap map;
    for(int i=0;i<chnList.count();i++)
    {
        if(!chnList[i].toMap()["enable"].toBool())
            continue;

        QString id=QString::number(chnList[i].toMap()["id"].toInt());
        map["name"]=chnList[i].toMap()["name"];
        map["id"] = id;


        QVariantList streams;
        streams << "stream" << "stream2";

        for(int j=0;j<streams.count();j++)
        {

            QVariantMap stream = chnList[i].toMap()[streams[j].toString()].toMap();
            QString suffix = "stream"+id;

            if(streams[j].toString() == "stream2")
                suffix = "sub"+id;

            if(stream.contains("suffix"))
                suffix = stream["suffix"].toString();


            QStringList urls;
            if(stream["http"].toBool())
                urls<<"http:///live/"+suffix;
            if(stream["hls"].toBool())
                urls<<"http:///hls/"+suffix+".m3u8";
            if(stream["rtmp"].toBool())
                urls<<"rtmp:///live/"+suffix;

            if(stream["rtsp"].type() == QVariant::Bool)
            {
                if(stream["rtsp"].toBool())
                    urls<<"rtsp:///"+suffix;
            }
            else
            {
                QVariantMap rtsp = stream["rtsp"].toMap();
                if(rtsp["enable"].toBool())
                {
                    if(rtsp["auth"].toBool())
                        urls<<"rtsp://"+rtsp["name"].toString()+":"+rtsp["passwd"].toString()+"@/"+suffix;
                    else
                        urls<<"rtsp:///"+suffix;
                }
            }

            if(stream["udp"].toMap()["enable"].toBool())
                urls<<(stream["udp"].toMap()["rtp"].toBool()?"rtp://@":"udp://@")+stream["udp"].toMap()["ip"].toString()+":"+QString::number(stream["udp"].toMap()["port"].toInt());
            if(stream["push"].toMap()["enable"].toBool())
                urls<<stream["push"].toMap()["path"].toString();

            if(streams[j].toString() == "stream")
            {
                map["url"]=urls.join("|");
                map["mainSuffix"] = suffix;
            }
            else
            {
                map["url2"]=urls.join("|");
                map["subSuffix"] = suffix;
            }
        }
        ret<<map;
    }

    return ret;
}

QVariantList RPC::getPlayList()
{
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="file")
        {
            ChannelFile *chn=(ChannelFile *)Config::chns[i];
            return chn->getPlayList();
        }
    }
    return QVariantList();
}

QVariantList RPC::getVolume()
{
    QVariantList ret;
    for(int i=0;i<Config::chns.count();i++)
    {
        QVariantMap map;
        if(Config::chns[i]->audio!=NULL && Config::chns[i]->enable)
        {
            QVariantMap data=Config::chns[i]->volume->invoke("getVolume").toMap();
            map["L"]=data["max"].toInt();
            if(data["avg"].toInt()<15)
                map["L"]=0;
            map["R"]=data["max2"].toInt();
            if(data["avg2"].toInt()<15)
                map["R"]=0;
            ret<<map;
        }
        else
        {
            map["L"]=0;
            map["R"]=0;
            ret<<map;
        }
    }
    return ret;
}

QVariantMap RPC::getPlayPosition()
{
    QVariantMap ret;
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="file")
        {
            ChannelFile *chn=(ChannelFile *)Config::chns[i];
            ret=chn->getPosition();
            break;
        }
    }
    return ret;
}

bool RPC::play(int index, int time)
{
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="file")
        {
            ChannelFile *chn=(ChannelFile *)Config::chns[i];
            return chn->play(index,time);
        }
    }
    return false;
}

QVariantList RPC::getPushSpeed()
{
    QVariantList ret;
    for(int i=0;i<Config::chns.count();i++)
    {
        if(!Config::chns[i]->enable)
            continue;
        ret<<Config::chns[i]->streamMap["push"]->mux->invoke("getSpeed").toMap()["speed"].toInt()*8/1024;
        ret<<Config::chns[i]->streamMap_sub["push"]->mux->invoke("getSpeed").toMap()["speed"].toInt()*8/1024;
    }

    return ret;
}

QString RPC::getSN()
{
    return Link::sn.toHex();
}

QVariantList RPC::getNDIList()
{
//    QVariantList list;
//    for(int i=0;i<Config::chns.count();i++)
//    {
//        if(Config::chns[i]->type=="ndi")
//        {
//            list=Config::chns[i]->encV->invoke("getList").toList();
//            break;
//        }
//    }
    QVariantList list;
    if(Channel::ndiRecv != NULL)
        list = Channel::ndiRecv->invoke("getList").toList();
    return list;
}

bool RPC::setNetDhcp(const bool &dhcp)
{
    if(!dhcp)
    {
        QVariantMap netMap = Json::loadFile("/link/config/net.json").toMap();
        netMap["dhcp"] = false;
        Json::saveFile(netMap,"/link/config/net.json");
        writeCom("/link/shell/setNetwork.sh");
        return true;
    }
    writeCom("udhcpc -i eth0 -q -s /link/shell/dhcp.sh");
    return true;
}

bool RPC::setTrans(QString json)
{
    QFile file("/link/config/trans.json");
    if(!file.open(QFile::ReadWrite))
        return false;
    file.resize(0);
    file.write(json.toUtf8());
    file.close();

    //    startTrans();

    return true;
}

bool RPC::updateColorKey()
{
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="colorKey")
        {
            Config::chns[i]->video->invoke("updatePoint");
            return true;
        }
    }
    return false;
}

bool RPC::pauseColorKey(const bool &pause)
{
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="colorKey")
        {
            Config::chns[i]->video->invoke("pause",pause);
            return true;
        }
    }
    return false;
}

void RPC::onTimerSyncRTC()
{
    if(QDate::currentDate().year()<2000)
        return;
    device->invoke("syncRTC2");
}

QString RPC::getChip()
{
    QString chip = "HI3521D";
#ifdef HI3531D
    chip = "HI3531D";
#elif HI3531DV200
    chip = "HI3531DV200";
#elif SS524V100
    chip = "SS524V100";
#elif SS528V100
    chip = "SS528V100";
#elif HI3559A
    chip = "HI3559A";
#elif HI3516E
    chip = "HI3516E";
#elif HI3519A
    chip = "HI3519A";
#elif SS626V100
    chip = "SS626V100";
#elif SS626V100
    chip = "HI3516CV610";
#endif
    return chip;
}

bool RPC::addOutputEdid(const QString &edidName)
{
    for(int i=0;i<Config::chns.count();i++)
    {
        if(Config::chns[i]->type=="mix")
        {
            ChannelMix *chn = (ChannelMix*)Config::chns[i];
            QString path="/link/config/edid/" + edidName;
            chn->outputV->invoke("saveEDID",path);
            return true;
        }
    }
    return false;
}

bool RPC::reloadConf()
{
    Config::loadConfig(CFGPATH);
    return true;
}

QString RPC::writeCom(const QString &com)
{
    QProcess proc;
    QStringList argList;
    argList << "-c" << com;
    proc.start("/bin/sh",argList);
    // 等待进程启动
    proc.waitForFinished();
    proc.waitForReadyRead();
    // 读取进程输出到控制台的数据
    QByteArray procOutput = proc.readAll();
    proc.close();
    return QString(procOutput);
}
