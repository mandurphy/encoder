#include "ChannelFile.h"
#include <QDateTime>

ChannelFile::ChannelFile(QObject *parent) : Channel(parent)
{
    inputFile=Link::create("InputFile");
    decA=Link::create("DecodeA");
    decV=Link::create("DecodeV");
    audio=Link::create("Resample");
    queue=Link::create("Queue");
    video=decV;
    encA=queue;
    encV=queue;
    encV2=Link::create("EncodeV");
    ea=Link::create("EncodeA");
    ev=Link::create("EncodeV");
    index=-1;
    connect(inputFile,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
}

void ChannelFile::init(QVariantMap)
{
    inputFile->linkV(queue);
    inputFile->linkA(queue);

    queue->linkV(decV)->linkV(overlay)->linkV(ev);
    overlay->linkV(encV2);

    queue->linkA(decA)->linkA(audio)->linkA(gain)->linkA(ea);
    decV->linkE(decA);

    Channel::init();
}

void ChannelFile::updateConfig(QVariantMap cfg)
{
    QVariantList list=cfg["file"].toList();
    playList.clear();
    for(int i=0;i<list.count();i++)
    {
       playList.append(list[i].toString());
    }

    if(cfg["enable"].toBool())
    {
        if(list.count()>0 && index==-1)
            playNext();

        QVariantMap dd;
        dd["delay"]=300;
        queue->start(dd);


        LinkObject *nextEncV=encV;
        if(cfg["decodeV"].toBool())
        {
            if(cfg["encv"].toMap()["codec"].toString()!="close")
            {
                nextEncV=ev;

                QVariantMap encvMap = cfg["encv"].toMap();
                ev->start(encvMap);
            }
            else
            {
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
            decV->start();
        }
        else
        {
            decV->stop();
            ev->stop();
            nextEncV=queue;
        }

        if(nextEncV!=encV)
        {
            qDebug()<<encV->name()<<" to "<<nextEncV->name();
            foreach(QString key,streamMap.keys())
            {
                encV->unLinkV(streamMap[key]->mux);
                streamMap[key]->mux->stop(key!="srt");
                nextEncV->linkV(streamMap[key]->mux);
            }
            encV=nextEncV;
        }


        LinkObject *nextEncA=encA;
        if(cfg["decodeA"].toBool())
        {
            decA->start();
            audio->start();

            if(cfg["enca"].toMap()["codec"].toString()=="close")
            {
                ea->stop();
                nextEncA=queue;
            }
            else
            {
                ea->start(cfg["enca"].toMap());
                nextEncA=ea;
            }

        }
        else
        {
            decA->stop();
            audio->stop();
            nextEncA=queue;
        }

        if(nextEncA!=encA)
        {
            qDebug()<<encA->name()<<" to "<<nextEncA->name();
            foreach(QString key,streamMap.keys())
            {
                encA->unLinkA(streamMap[key]->mux);
                streamMap[key]->mux->stop(key!="srt");
                nextEncA->linkA(streamMap[key]->mux);

                if(streamMap_sub.contains(key))
                {
                    encA->unLinkA(streamMap[key]->mux);
                    streamMap[key]->mux->stop(key!="srt");
                    nextEncA->linkA(streamMap[key]->mux);
                }
            }
            encA=nextEncA;
        }

    }
    else
    {
        index=-1;
        inputFile->stop();
        queue->stop();
        ea->stop();
        ev->stop();
        decA->stop();
        decV->stop();
        audio->stop();
    }
    Channel::updateConfig(cfg);
}

QVariantList ChannelFile::getPlayList()
{
    QVariantList ret;
    for(int i=0;i<playList.count();i++)
    {
        QVariantMap map;
        QString name=playList[i];
        map["name"]=name;
        if(!durationMap.contains(name))
        {
            durationMap[name]=inputFile->invoke("getDuration",fullPath(name)).toLongLong();
        }
        map["duration"]=durationMap[playList[i]];
        ret<<map;
    }
    return ret;
}

bool ChannelFile::seek(int index, qint64 time)
{
    this->index=index;
    QVariantMap dd;
    dd["path"]=playList[index];
    inputFile->start(dd);
    inputFile->invoke("seek",time);
    return true;
}

QVariantMap ChannelFile::getPosition()
{
    QVariantMap ret;
    ret["file"]=file;
    ret["position"]=inputFile->invoke("getPosition").toInt();
    return ret;
}

bool ChannelFile::play(int index, int time)
{
    this->index =index-1;
    playNext();
    inputFile->invoke("seek",time);
    return true;
}

void ChannelFile::playNext()
{
    index=(index+1)%playList.count();
    QVariantMap dd;
    file=playList[index];
    dd["path"]=fullPath(playList[index]);
    qDebug()<<"Play"<< dd["path"].toString();
    inputFile->start(dd);

}

QString ChannelFile::fullPath(QString path)
{
    return "/root/usb/"+path;
}

void ChannelFile::onNewEvent(QString type, QVariant )
{
    qDebug()<<type;
    if(type!="EOF")
        return;

    playNext();
}

