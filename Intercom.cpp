#include "Intercom.h"
#include <QFile>
#include "Json.h"

Intercom::Intercom(QObject *parent) : QObject(parent),timer(this)
{
    connect(&timer,SIGNAL(timeout()),this,SLOT(onTimer()));
    ai=NULL;
}

void Intercom::init()
{

    if(!QFile::exists("/link/config/intercom.json"))
        return;

    intercom=Link::create("Intercom");

    gain=Link::create("Gain");


    server=Link::create("IntercomServer");
    vmix=Link::create("VMix");
    tally=Link::create("Tally");

    vmix->linkE(intercom);
    connect(intercom,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
    update(Json::loadFile("/link/config/intercom.json").toMap());
}

bool Intercom::update(QVariantMap cfg)
{
    QVariantMap cfgIntercom=cfg["intercom"].toMap();
    if(ai==NULL && cfgIntercom["enable"].toBool())
    {
        QVariantMap data;
        ai=Link::create("InputAlsa");
        data["path"] = "-1";
        data["id"]="0d8c:0014|12d1:0010|0112:0200";
        ai->start(data);
        ao=Link::create("OutputAlsa");
        ao->start(data);

        res1=Link::create("Resample");
        data.clear();
        data["num"]=200;
        data["channels"]=1;
        data["samplerate"]=8000;
        res1->start(data);
        res2=Link::create("Resample");
        res2->start();

        ai->linkA(res1)->linkA(gain)->linkA(intercom)->linkA(res2)->linkA(ao);

    }




    if(cfgIntercom["enable"].toBool())
    {
        if(cfgIntercom["vad"].toInt()>0)
            cfgIntercom["vad"]=cfgIntercom["vad"].toInt()+cfgIntercom["gain"].toInt();

        intercom->start(cfgIntercom);
        QVariantMap cfgGain;
        cfgGain["gain"]=cfgIntercom["gain"].toInt();
        gain->start(cfgGain);
    }
    else
        intercom->stop();



    QVariantMap cfgServer=cfg["server"].toMap();
    if(cfgServer["enable"].toBool())
        server->start();
    else
        server->stop();

    QVariantMap cfgVMix=cfg["vmix"].toMap();
    if(cfgVMix["enable"].toBool())
        vmix->start(cfgVMix);
    else
        vmix->stop();

    QVariantMap cfgTally=cfg["tally"].toMap();
    if(cfgTally["enable"].toBool())
    {
        timer.start(1000);
        tally->start(cfgTally);
    }
    else
    {
        timer.stop();
        tally->stop();
    }

    Json::saveFile(cfg,"/link/config/intercom.json");
    return true;
}

bool Intercom::setTally(QVariantList list)
{
    tally->invoke("setTally",list);
    tallyInfo=list;
    emit this->newEvent("TALLY",tallyInfo);
    return true;
}

QVariantMap Intercom::getState()
{
    QVariantMap state;
    QVariantMap map=intercom->invoke("getState").toMap();
    state["intercom"]=map["list"].toList();
    state["talking"]=map["talking"].toBool();
    state["tally"]=tallyInfo;
    return state;
}

void Intercom::onTimer()
{
    if(tallyInfo.isValid())
    {
        tally->invoke("setTally",tallyInfo);
    }
}

void Intercom::onNewEvent(QString type, QVariant info)
{
    if(type=="TALLY")
    {
        tallyInfo=info;
        tally->invoke("setTally",info);
        emit this->newEvent("TALLY",tallyInfo);
    }
}

