#include "Channel.h"
#include "unistd.h"
#include <QFile>
#include <QDir>
#include <QDateTime>
#include "Json.h"
#include "Config.h"
#include <QProcess>
#include <QPair>
#include "ChannelNet.h"

LinkObject* Stream::httpServer=NULL;
LinkObject* Stream::rtspServer=NULL;
LinkObject* Stream::webrtcServer=NULL;
LinkObject *Channel::lineIn=NULL;
LinkObject *Channel::lineOut=NULL;
LinkObject *Channel::alsa=NULL;
LinkObject *Channel::ndiRecv=NULL;
Channel::Channel(QObject *parent) :
    QObject(parent)
{
#ifdef HI3516CV610
    overlay=Link::create("Overlay");
#else
    overlay=Link::create("Overlay2");
#endif

    volume=Link::create("Volume");
    snap=Link::create("EncodeV");
    gain=Link::create("Gain");

    enable=false;
    enableAVS=false;
    isSrcLine=false;
    audio=NULL;
    video=NULL;
    encA=NULL;
    encV=NULL;
    encV2=NULL;
    id=-1;
    lastAId = -1;
    chnName="";

    cd_pauseTimer = new QTimer;
    connect(cd_pauseTimer,SIGNAL(timeout()),SLOT(cdPauseTimeout()));
}

void Channel::init(QVariantMap cfg)
{
    if(lineIn==NULL)
    {
        QString name="Line-In";
        if(QFile::exists("/dev/tlv320aic31"))
            name="Mini-In";
        QVariantMap ifaceA=Json::loadFile("/link/config/board.json").toMap()["interfaceA"].toMap();
        if(ifaceA.keys().contains(name))
        {
            if(ifaceA[name].toMap().contains("alsa") || ifaceA[name].toMap().contains("bus"))
            {
                alsa=Link::create("InputAlsa");
                QVariantMap alsaData=ifaceA[name].toMap();
                alsaData["path"]=alsaData["alsa"].toString();
                alsa->start(alsaData);
                lineIn=Link::create("Resample");
                lineIn->start();
                alsa->linkA(lineIn);
            }
            else
            {
                lineIn=Link::create("InputAi");
                QVariantMap dd;
                dd["interface"]=name;
                lineIn->start(dd);
            }
        }
    }

#if !defined HI3516E && !defined HI3516CV610
    if(ndiRecv == NULL)
    {
        ndiRecv=Link::create("NDIRecv");
        ndiRecv->start();
    }
#endif

    if(video!=NULL)
    {
        video->linkV(overlay)->linkV(snap);
    }

    if(audio!=NULL)
    {
        gain->linkA(volume);
    }

    QVariantMap path;
    if(encA==NULL)
        path["mute"]=true;

    QString mainSuffix = "stream" + QString::number(id);
    QString subSuffix = "sub" + QString::number(id);
    for (const QString &item : {"stream", "stream2"}) {
        QVariantMap stream_map = cfg[item].toMap();
        QString suffix = stream_map["suffix"].toString();
        if (!suffix.isEmpty()) {
            if (item == "stream")
                mainSuffix = suffix;
            else if (item == "stream2")
                subSuffix = suffix;
        }
    }

    QList<QString> protocols= QList<QString>({"rtmp", "push","hls","srt","ts","rtsp","webrtc","ndi"});
#ifdef HI3516CV610
    protocols.removeLast();
#endif

    for (const QString &item :protocols)
    {
        streamMap[item] = new Stream(item, mainSuffix,id+"");
        if(item != "ndi")
            streamMap_sub[item] = new Stream(item, subSuffix,id+"");

        if(encA!=NULL)
        {
            encA->linkA(streamMap[item]->mux);
            if(streamMap_sub.contains(item))
                encA->linkA(streamMap_sub[item]->mux);
        }
        encV->linkV(streamMap[item]->mux);
        if(encV2!=NULL && streamMap_sub.contains(item))
            encV2->linkV(streamMap_sub[item]->mux);
    }
}

void Channel::updateConfig(QVariantMap cfg)
{
    data=cfg;
    enable=cfg["enable"].toBool();
    bool enable2=cfg["enable2"].toBool();

    if(enable || enable2)
    {
        QVariantMap sd;
        sd["width"]=640;
        sd["height"]=360;
        sd["qfactor"]=99;
        sd["codec"]="jpeg";

        if(type=="vi" )
        {
#ifdef HI3516E
            sd["scaleUp"]=true;
#endif
            sd["share"]=0;
        }
        snap->start(sd);
        volume->start();

        if(cfg.contains("enca"))
        {
            QVariantMap gd;
            gd["gain"]=cfg["enca"].toMap()["gain"];
            gain->start(gd);
        }
        else
            gain->start();

        QVariantMap layMap;
        layMap["width"] = cfg["encv"].toMap()["width"].toInt();
        layMap["height"] = cfg["encv"].toMap()["height"].toInt();
        overlay->start(layMap);

        if(cfg.contains("enca"))
        {
            QVariantMap cfga=cfg["enca"].toMap();
            if(cfga.contains("audioSrc"))
            {
                bool isNum = false;
                cfga["audioSrc"].toInt(&isNum);
                if(!isNum)
                {
                    QString audioSrc = cfga["audioSrc"].toString();
                    if(audioSrc == "line" && !isSrcLine)
                    {
                        if(lineIn!=NULL)
                        {
                            Channel* lastChn = Config::findChannelById(lastAId);
                            if(lastChn != NULL)
                                lastChn->audio->unLinkA(gain);
                            lineIn->linkA(gain);
                        }
                        isSrcLine = true;
                    }

                    if(audioSrc == "hdmi" || audioSrc == "sdi")
                    {
                        if(lineIn!=NULL)
                            lineIn->unLinkA(gain);

                        if(lastAId != -1)
                        {
                            Channel* lastChn = Config::findChannelById(lastAId);
                            if(lastChn != NULL && lastChn->audio != NULL)
                                lastChn->audio->unLinkA(gain);
                        }
                        Channel* chn = Config::findChannelById(id);
                        if(chn != NULL && chn->audio != NULL)
                            chn->audio->linkA(gain);

                        lastAId = id;
                        isSrcLine = false;
                    }
                }
                else
                {

                    if(lineIn!=NULL)
                        lineIn->unLinkA(gain);

                    int audioSrc = cfga["audioSrc"].toInt();
                    if(audioSrc != lastAId || isSrcLine)
                    {
                        if(lastAId != -1)
                        {
                            Channel* lastChn = Config::findChannelById(lastAId);
                            if(lastChn != NULL && lastChn->audio != NULL)
                                lastChn->audio->unLinkA(gain);
                        }

                        Channel* chn = Config::findChannelById(audioSrc);
                        if(chn != NULL && chn->audio != NULL)
                            chn->audio->linkA(gain);

                        lastAId = audioSrc;
                        isSrcLine = false;
                    }
                }
            }
        }
        else
        {
            if(audio!=NULL)
                audio->linkA(gain);
        }
    }
    else
    {
        snap->stop();
        volume->stop();
    }


    for (const QString &item : QList<QString>({"stream", "stream2"}))
    {
        QVariantMap stream_cfg = cfg[item].toMap();
        bool stream_enable = (item == "stream") ? enable : enable2;
        QMap<QString,Stream*> stream_map = (item == "stream") ? streamMap : streamMap_sub;
        QString stream_encv = (item == "stream") ? "encv" : "encv2";

        QVariantMap muxData;
        if (type == "net")
        {
            ChannelNet *chnNet = dynamic_cast<ChannelNet*>(this);
            bool haveAudio = chnNet->haveAudio;
            bool haveVideo = (item == "stream2") ? chnNet->haveVideo2 : chnNet->haveVideo;
            muxData["mute"] = !haveAudio;
            muxData["noVideo"] = !haveVideo;
        }
        else
        {
            muxData["mute"] = (cfg["enca"].toMap()["codec"].toString() == "close");
            muxData["noVideo"] = (cfg[stream_encv].toMap()["codec"].toString() == "close");
        }

        for(int i=0;i<stream_map.keys().count();i++)
        {
            QString stream_type = stream_map.keys()[i];

            if(stream_type == "ndi" || stream_type == "push" || stream_type == "srt")
                continue;

            if(stream_cfg.contains("suffix"))
            {
                QString suffix = stream_cfg["suffix"].toString();
                if(stream_map[stream_type]->streamSuffix != suffix)
                {
                    QString path = stream_map[stream_type]->streamPath;
                    if(!path.isEmpty())
                    {
                        path.replace(stream_map[stream_type]->streamSuffix,suffix);
                        muxData["path"] = path;
                    }
                    stream_map[stream_type]->streamPath = path;
                    stream_map[stream_type]->streamSuffix = suffix;
                }
            }
            stream_map[stream_type]->mux->setData(muxData);
        }

        if(stream_enable && stream_cfg["rtmp"].toBool())
            stream_map["rtmp"]->mux->start();
        else
            stream_map["rtmp"]->mux->stop();

        if(stream_enable && stream_cfg["hls"].toBool())
        {
            stream_map["hls"]->mux->start(cfg["hls"].toMap());
        }
        else
        {
            stream_map["hls"]->mux->stop();
            writeCom("rm -rf /tmp/hls/"+stream_map["hls"]->streamSuffix+"*");
        }

        if(stream_enable && stream_cfg["srt"].toMap()["enable"].toBool())
        {
            QVariantMap cfg=stream_cfg["srt"].toMap();
            QVariantMap dd;
            QString ip=cfg["ip"].toString();
            QString mode=cfg["mode"].toString();
            if(mode=="listener")
                ip="0.0.0.0";

            QString url="srt://"+ip+":" + QString::number(cfg["port"].toInt())+"?mode="+mode+"&latency="+ QString::number(cfg["latency"].toInt());

            if(!cfg["streamid"].toString().isEmpty())
                url="srt://"+cfg["ip"].toString()+":" + QString::number(cfg["port"].toInt())+"?streamid="+cfg["streamid"].toString();

            if(!cfg["passwd"].toString().isEmpty())
                url+="&passphrase="+cfg["passwd"].toString();
            dd["path"]=url;
            stream_map["srt"]->mux->start(dd);
        }
        else
            stream_map["srt"]->mux->stop();


        if(stream_enable &&  (stream_cfg["http"].toBool()  || stream_cfg["udp"].toMap()["enable"].toBool()))
        {
            if(cfg.contains("ts"))
            {
                QVariantMap ts_cfg=cfg["ts"].toMap();
                ts_cfg["service_name"]=cfg["name"].toString();
                stream_map["ts"]->mux->start(ts_cfg);
            }
            else
                stream_map["ts"]->mux->start();
        }
        else
            stream_map["ts"]->mux->stop();

        if(stream_cfg["rtsp"].type() == QMetaType::Bool)
        {
            if(stream_enable && stream_cfg["rtsp"].toBool() )
            {
                stream_map["rtsp"]->mux->start();
                stream_map["rtsp"]->mux->linkV(Stream::rtspServer);
                stream_map["rtsp"]->mux->linkA(Stream::rtspServer);
            }
            else
            {
                stream_map["rtsp"]->mux->stop();
                stream_map["rtsp"]->mux->unLinkV(Stream::rtspServer);
                stream_map["rtsp"]->mux->unLinkA(Stream::rtspServer);
            }
        }
        else
        {
            QVariantMap rtsp = stream_cfg["rtsp"].toMap();
            if(stream_enable && rtsp["enable"].toBool())
            {
                QVariantMap rtspData;
                QString name = "";
                QString passwd = "";
                if(rtsp["auth"].toBool())
                {
                    name = rtsp["name"].toString();
                    passwd = rtsp["passwd"].toString();
                }
                rtspData["name"] = name;
                rtspData["passwd"] = passwd;
                stream_map["rtsp"]->mux->linkV(Stream::rtspServer);
                stream_map["rtsp"]->mux->linkA(Stream::rtspServer);

                stream_map["rtsp"]->mux->start(rtspData);

            }
            else
            {
                stream_map["rtsp"]->mux->stop();
                stream_map["rtsp"]->mux->unLinkV(Stream::rtspServer);
                stream_map["rtsp"]->mux->unLinkA(Stream::rtspServer);
            }
        }

        if(stream_enable && stream_map.contains("webrtc") && Stream::webrtcServer != NULL)
        {
            if(stream_cfg["webrtc"].toBool())
            {
                stream_map["webrtc"]->mux->start();
                stream_map["webrtc"]->mux->linkV(Stream::webrtcServer);
                stream_map["webrtc"]->mux->linkA(Stream::webrtcServer);
            }
            else
            {
                stream_map["webrtc"]->mux->stop();
                stream_map["webrtc"]->mux->unLinkV(Stream::webrtcServer);
                stream_map["webrtc"]->mux->unLinkA(Stream::webrtcServer);
            }
        }

        if(stream_enable && stream_cfg["http"].toBool())
            stream_map["ts"]->mux->linkV(Stream::httpServer);
        else
            stream_map["ts"]->mux->unLinkV(Stream::httpServer);


        if(stream_enable && stream_cfg["udp"].toMap()["enable"].toBool())
            stream_map["ts"]->udpServer->start(stream_cfg["udp"].toMap());
        else
            stream_map["ts"]->udpServer->stop();

        if(stream_enable && stream_cfg["push"].toMap()["enable"].toBool())
            stream_map["push"]->mux->start(stream_cfg["push"].toMap());
        else
            stream_map["push"]->mux->stop();

    #if !defined HI3516E && !defined HI3516CV610
        if(item == "stream")
        {
            QVariantMap ndi_cfg=cfg["ndi"].toMap();
            if(stream_enable && ndi_cfg["enable"].toBool())
            {
                stream_map["ndi"]->mux->start(ndi_cfg);
                if(encV!=NULL)
                    encV->linkV(stream_map["ndi"]->mux);
                if(encV2!=NULL)
                    encV2->linkV(stream_map["ndi"]->mux);
            }
            else
                stream_map["ndi"]->mux->stop();
        }
    #endif
    }
}

QString Channel::writeCom(const QString &com)
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


QString Channel::doSnap(const int &mod, const QVariantList &chnIds)
{
    QString path="/tmp/snap/snap"+QString::number(id)+".jpg";
    QVariantMap sd;
    if(chnIds.isEmpty() || chnIds.contains(id))
    {
        if(mod == 1)
        {
            sd["width"]=-1;
            sd["height"]=-1;
            snap->setData(sd);
            path = "/tmp/snap/sync_snap"+QString::number(id)+".jpg";
            snap->invoke("snapSync",path);
        }
        else
        {
            sd["width"]=640;
            sd["height"]=360;
            snap->setData(sd);
            snap->invoke("snap",path);
        }
    }
    return path.mid(4);
}

int Channel::timerStrToInt(QString time)
{
    int count_time = 0;
    if(!time.isEmpty())
    {
        time = time.replace("：",":");
        QStringList timeList = time.split(":");
        for(int j=0;j<timeList.count();j++)
        {
            QString str = timeList[j];
            if(j == 0)
                count_time += str.toInt()*3600;
            if(j == 1)
                count_time += str.toInt()*60;
            if(j == 2)
                count_time += str.toInt();
        }
    }
    else
        count_time = 99999999;
    return count_time;
}

void Channel::startRecord(const QString &fileName, const QString &format, QVariantMap fragment)
{
    if(formatMap.contains(format))
    {
        QString mark = formatMap[format]->property("record").toString();
        if(mark == "on")
            return;
    }
    QStringList infoList = fileName.split("/");
    QString filePath;
    for(int i=0;i<infoList.count();i++)
    {
        if(i == infoList.count() - 1)
            continue;
        filePath = filePath+infoList[i]+"/";
    }

    QDir pathDir(filePath);
    if(!pathDir.exists())
        pathDir.mkpath(filePath);

    QString jpg=fileName+".jpg";;
    snap->invoke("snapSync",jpg);
    snap->invoke("snapSync",jpg);

    if(!formatMap.contains(format))
        formatMap[format] = Link::create("Mux");

    QVariantMap dd;
    LinkObject *mux = formatMap[format];

    if(!fragment.isEmpty())
    {
        bool duraEnable = fragment["segmentDuraEnable"].toBool();
        bool sizeEnable = fragment["segmentSizeEnable"].toBool();
        if(duraEnable)
        {
            dd["segmentDuration"] = fragment["segmentDura"].toFloat()*1000*3600;
            dd["segmentSize"] = 0;
            dd["path"] = fileName+"_%d."+format;
        }
        if(sizeEnable)
        {
            dd["segmentDuration"] = 0;
            dd["segmentSize"] = fragment["segmentSize"].toFloat()*1024*1024*1024;
            dd["path"] = fileName+"_%d."+format;
        }
        if(!duraEnable && !sizeEnable)
        {
            dd["segmentDuration"] = 0;
            dd["segmentSize"] = 0;
            dd["path"] = fileName+"_0."+format;

        }
        dd["startNum"] = 0;
    }
    else
    {
        dd["path"] = fileName+"."+format;
    }

    if(encA==NULL || encA->getState()!="started")
        dd["mute"]=true;
    else
        encA->linkA(mux);

    encV->linkV(mux);

    if(format=="mp4")
        dd["filecache"]=20480000;

    mux->start(dd);
    mux->setProperty("record","on");

    if(startRecordTime.isEmpty())
    {
        QDateTime curTime = QDateTime::currentDateTime();
        startRecordTime = QString::number(curTime.toTime_t());
    }
}

void Channel::stopRecord(const QString &format)
{
    if(!formatMap.contains(format))
        return;

    int offCount = 0;
    QMap<QString,LinkObject*>::Iterator it;
    for(it = formatMap.begin();it != formatMap.end();++it)
    {
        LinkObject *mux = it.value();
        QString mark = formatMap[it.key()]->property("record").toString();
        if(format == it.key())
        {
            if(mark == "off")
                return;
            mux->stop();
            formatMap[format]->setProperty("record","off");
            mark = "off";
        }
        if(mark == "off")
            offCount++;
    }
    if(formatMap.count() == offCount)
    {
        startRecordTime = "";
        pauseTime = 0;
    }
}

void Channel::recordPuase(const bool &pause)
{
    QMap<QString,LinkObject*>::Iterator it;
    for(it = formatMap.begin();it != formatMap.end();++it)
    {
        LinkObject *mux = it.value();
        if(pause)
        {
            mux->invoke("pause");
            cd_pauseTimer->start(1000);
        }
        else
        {
            mux->invoke("resume");
            cd_pauseTimer->stop();
        }
    }
}

void Channel::cdPauseTimeout()
{
    pauseTime++;
}


