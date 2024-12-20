#include "ChannelMix.h"
#include "Config.h"
#include "ChannelVI.h"
#include <QFile>
#include "Json.h"

ChannelMix::ChannelMix(QObject *parent) : Channel(parent)
{
    audio=Link::create("MixA");
    video=Link::create("MixV");
    encA=Link::create("EncodeA");
    encV=Link::create("EncodeV");
    encV2=Link::create("EncodeV");
    lastSrcV=NULL;
    lastSrcV2=NULL;
    lastSrcA=NULL;
    lastSrcA2=NULL;
    lastSrcALine=NULL;
    lineOutGain=NULL;
    hadOutputLine=true;
}

void ChannelMix::init(QVariantMap cfg)
{
    Q_UNUSED(cfg);
    gain->linkA(encA);
    overlay->linkV(encV);
    overlay->linkV(encV2);

    outputV=Link::create("OutputVo");
    outputV2=Link::create("OutputVo");
    outputA=Link::create("OutputAo");
    outputA2=NULL;
    QVariantMap aoData;
    aoData["interface"]="HDMI-OUT";
    outputA->setData(aoData);
    audio->linkA(outputA);


    hadOutputLine = cfg.contains("outputLine");
    QString name="Line-Out";
    if(QFile::exists("/dev/tlv320aic31"))
        name="Mini-Out";
    QVariantMap ifaceA=Json::loadFile("/link/config/board.json").toMap()["interfaceA"].toMap();
    if(ifaceA.keys().contains(name))
    {
        if(ifaceA[name].toMap().contains("alsa") || ifaceA[name].toMap().contains("bus"))
        {

            lineOut=Link::create("OutputAlsa");
            QVariantMap alsaData=ifaceA[name].toMap();
            alsaData["path"]=alsaData["alsa"].toString();
            lineOut->setData(alsaData);
        }
        else
        {
            lineOut=Link::create("OutputAo");
            aoData["interface"]=name;
            lineOut->setData(aoData);
        }

        if(hadOutputLine)
        {

            int gain = cfg["outputLine"].toMap()["gain"].toInt();
            lineOutGain = Link::create("Gain");
            lineOutGain->start(QVariantMap{{"gain", gain}});

            QString srcA = cfg["outputLine"].toMap()["src"].toString();
            if(srcA == "line")
                lastSrcALine = ChannelVI::lineIn;
            else
                lastSrcALine=Config::findChannelById(srcA.toInt())->gain;

            if(lastSrcALine != NULL)
                lastSrcALine->linkA(lineOutGain)->linkA(lineOut);
            lineOut->start();
        }
        else
            audio->linkA(lineOut);
    }

    if(ifaceA.keys().contains("SDI-OUT") || ifaceA.keys().contains("HDMI-OUT2"))
    {
        outputA2=Link::create("OutputAo");
        if(ifaceA.keys().contains("SDI-OUT"))
            aoData["interface"]="SDI-OUT";
        else if(ifaceA.keys().contains("HDMI-OUT2"))
            aoData["interface"]="HDMI-OUT2";
        outputA2->setData(aoData);
        audio->linkA(outputA2);
    }

    lastSrcA=audio;
    lastSrcA2=audio;

    Channel::init();
}

void ChannelMix::updateConfig(QVariantMap cfg)
{
    if(cfg["enable"].toBool())
    {

        QVariantMap dataMixV;
        if(cfg["encv"].toMap()["width"].toInt()!=-1)
        {
            dataMixV["width"]=cfg["encv"].toMap()["width"].toInt();
            dataMixV["height"]=cfg["encv"].toMap()["height"].toInt();
            dataMixV["framerate"]=cfg["encv"].toMap()["framerate"].toInt();
        }

        video->start(dataMixV);
        audio->start();

        QVariantList srcV=cfg["srcV"].toList();
        QVariantList srcA=cfg["srcA"].toList();
        QVariantList videoList;
        for(int i=0;i<srcV.count();i++)
        {

            if(srcV[i].toInt()!=-1)
            {
                LinkObject *v=Config::findChannelById(srcV[i].toInt())->overlay;

                videoList.append(v->name());
                v->linkV(video);
            }
            else
            {
                videoList.append("unknow");
            }
        }


        dataMixV["src"]=videoList;
        dataMixV["layout"]=cfg["layout"].toList();

        foreach(int id,curAList)
        {
            if(!srcA.contains(id))
            {
                LinkObject *a=Config::findChannelById(id)->gain;
                a->unLinkA(audio);
                curAList.removeAll(id);
            }
        }

        QVariantMap dataMixA;
        dataMixA["bufLen"]=20;
        for(int i=0;i<srcA.count();i++)
        {
            if(srcA[i]==-1)
                continue;
            Channel *chn=Config::findChannelById(srcA[i].toInt());
            if(chn->audio==NULL )
                continue;
            LinkObject *a=chn->gain;
            if(!dataMixA.contains("main"))
                dataMixA["main"]=a->name();
            a->linkA(audio);
            curAList.append(srcA[i].toInt());
        }

        if(ChannelVI::lineIn!=NULL)
        {
            ChannelVI::lineIn->linkA(audio);
            dataMixA["main"]=ChannelVI::lineIn->name();
        }

        audio->setData(dataMixA);
        video->setData(dataMixV);

        if(cfg["enca"].toMap()["codec"].toString()!="close")
            encA->start(cfg["enca"].toMap());
        else
            encA->stop();

        if(cfg["encv"].toMap()["codec"].toString()!="close")
        {
            encV->start(cfg["encv"].toMap());
        }
        else
            encV->stop();

        if(cfg["encv"].toMap()["codec"].toString()!="close")
        {
            encV->start(cfg["encv"].toMap());
        }
        else
            encV->stop();

        if(cfg["enable2"].toBool()  && cfg["encv2"].toMap()["codec"].toString()!="close")
            encV2->start(cfg["encv2"].toMap());
        else
            encV2->stop();

    }
    else
    {
        audio->stop();
        video->stop();
        encA->stop();
        encV->stop();
        encV2->stop();
    }

    QVariantMap outCfg=cfg["output"].toMap();
    if(outCfg["enable"].toBool())
    {
        outputA->start();
        if(!hadOutputLine && lineOut!=NULL)
            lineOut->start();

        Channel *chn=Config::findChannelById(outCfg["src"].toInt());
        if(chn->type=="vi")
            outCfg["scaleUp"]=true;
        outputV->start(outCfg);
        LinkObject *v=chn->overlay;
        LinkObject *a=chn->gain;
        if(v!=NULL)
        {
            if(v!=lastSrcV && lastSrcV!=NULL)
                lastSrcV->unLinkV(outputV);
            lastSrcV=v;
            v->linkV(outputV);
        }

        if(a!=NULL)
        {
            if(a!=lastSrcA && lastSrcA!=NULL)
            {
                lastSrcA->unLinkA(outputA);
                a->linkA(outputA);
                if(!hadOutputLine && lineOut!=NULL)
                {
                    lastSrcA->unLinkA(lineOut);
                    if(lastSrcA2!=NULL)
                        lastSrcA2->unLinkA(lineOut);
                    a->linkA(lineOut);
                }
            }
            lastSrcA=a;
        }
    }
    else
        outputV->stop();


    QVariantMap outCfg2=cfg["output2"].toMap();
    if(outCfg2["enable"].toBool())
    {
        if(outputA2!=NULL)
            outputA2->start();

        if(!hadOutputLine && lineOut!=NULL && !outCfg["enable"].toBool())
            lineOut->start();

#if SS524V100
        if(outCfg2.contains("chip"))
        {
            QVariantMap boardV=Link::getConfig()["interfaceV"].toMap();
            if(boardV.contains("SDI")
                    && boardV["SDI"].toMap().contains("chip")
                    && boardV["SDI"].toMap()["chip"].toString()=="fpga")
            {
                outCfg2.remove("chip");
                outCfg2.remove("swapYC");
            }
        }
#endif


        Channel *chn=Config::findChannelById(outCfg2["src"].toInt());
        if(chn->type=="vi")
            outCfg2["scaleUp"]=true;
        outputV2->start(outCfg2);
        LinkObject *v=chn->overlay;
        LinkObject *a=chn->gain;
        if(v!=NULL)
        {
            if(v!=lastSrcV2 && lastSrcV2!=NULL)
                lastSrcV2->unLinkV(outputV2);
            lastSrcV2=v;
            v->linkV(outputV2);
        }

        if(a!=NULL && outputA2!=NULL)
        {
            if(a!=lastSrcA2 && lastSrcA2!=NULL)
            {
                lastSrcA2->unLinkA(outputA2);
                a->linkA(outputA2);
                if(!hadOutputLine && lineOut!=NULL && !outCfg["enable"].toBool())
                {
                    lastSrcA2->unLinkA(lineOut);
                    if(lastSrcA!=NULL)
                        lastSrcA->unLinkA(lineOut);
                    a->linkA(lineOut);
                }
            }
            lastSrcA2=a;
        }

#if HI3531DV200 || SS528V100
        if(outCfg2.contains("chip") && outCfg2["chip"].toString()=="gs2972")
        {

        }
        else if(outCfg2["type"].toString().contains("bt1120"))
        {
            static int lastNorm=0;
            int norm=0;
            int ddr=0;
            QString str=outCfg2["output"].toString();
            if(str=="1080P60")
                norm=9;
            else if(str=="1080P50")
                norm=10;
            else if(str=="1080P30")
                norm=12;
            else if(str=="720P60")
                norm=5;
            else if(str=="720P50")
                norm=6;
            else if(str=="3840x2160_30")
            {
                norm=14;
                ddr=1;
            }

            if(norm!=lastNorm)
            {
                lastNorm=norm;
                QString cmd="rmmod hi_lt8618sx_lp.ko";
                system(cmd.toLatin1().data());
                cmd=cmd.sprintf("insmod /ko/extdrv/hi_lt8618sx_lp.ko norm=%d USE_DDRCLK=%d",norm,ddr);
                system(cmd.toLatin1().data());
            }
        }
#endif
    }
    else
        outputV2->stop();


    if(hadOutputLine)
    {
        QVariantMap outLineCfg = cfg["outputLine"].toMap();
        QString srcA = outLineCfg["src"].toString();
        LinkObject *curLineSrcA = NULL;
        if(srcA == "line")
            curLineSrcA = ChannelVI::lineIn;
        else
            curLineSrcA=Config::findChannelById(srcA.toInt())->gain;

        if(lineOut != NULL && lastSrcALine != NULL && lineOutGain !=NULL && lastSrcALine != NULL)
        {
            if(lastSrcALine != curLineSrcA)
            {
                lastSrcALine->unLinkA(lineOutGain);
                curLineSrcA->linkA(lineOutGain);
                lastSrcALine = curLineSrcA;
            }
            lineOutGain->setData(QVariantMap{{"gain", outLineCfg["gain"].toInt()}});
        }
    }

    Channel::updateConfig(cfg);
}

