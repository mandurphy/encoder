#include "Capture.h"
#include "Json.h"
#include <QFile>
#include "Config.h"

Capture::Capture(QObject *parent) : QObject(parent)
{

}

void Capture::init()
{
    if(!QFile::exists(CAPTUREFILE))
        return;

    QVariantMap conf = Json::loadFile(CAPTUREFILE).toMap();

    enc=Link::create("EncodeV");
    res=Link::create("Resample");
    uvc=Link::create("OutputUVC");
    uac=Link::create("OutputAlsa");

    connect(uvc,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
    update(conf);
}

bool Capture::update(QVariantMap cfg)
{
    if(cfg.isEmpty())
        return false;

    config=cfg;

    int srcId=config["src"].toInt();
    Channel *chn=Config::findChannelById(srcId);
    if(chn==NULL)
        return false;

    srcV=chn->overlay;
    srcA=chn->gain;

    QVariantMap data;
    data["lnk"]=config["lnk"];
    uac->start(data);

    res->start();
    uvc->start();

    data.clear();
    data["codec"]="jpeg";
    data["snap"]=false;
    data["framerate"]=config["framerate"];
    data["memsize"]=2048000;
    data["bitrate"]=config["bitrate"];
    data["share"]=0;
    enc->start(data);

    srcV->linkV(enc)->linkV(uvc);
    srcA->linkA(res)->linkA(uac);

    Json::saveFile(cfg,CAPTUREFILE);
    return true;
}

void Capture::onNewEvent(QString type, QVariant info)
{
    if(type=="resize" && enc!=NULL)
    {
        enc->setData(info.toMap());
    }
}
