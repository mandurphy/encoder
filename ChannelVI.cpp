#include "ChannelVI.h"
#include <math.h>
#include <QFile>
#include "Config.h"

ChannelVI::ChannelVI(QObject *parent) :
    Channel(parent)
{

    vi=Link::create("InputVi");
    encA=Link::create("EncodeA");
    encV=Link::create("EncodeV");
    encV2=Link::create("EncodeV");
    alsa=NULL;
    ipcAo=NULL;
#if (defined HI3519A) || (defined HI3559A)
    viR=Link::create("InputVi");
    AVS=Link::create("SAVS");
    video=AVS;
#else
    video=vi;
#ifdef HI3516CV610
    ipcAo = Link::create("OutputAo");
    dei = NULL;
#elif SS928V100
    dei = NULL;
#else
    dei = Link::create("Deinterlace");
#endif


#endif

}

void ChannelVI::init(QVariantMap cfg)
{
    if(cfg.contains("alsa"))
    {
        alsa=Link::create("InputAlsa");
        audio=Link::create("Resample");
        audio->start();
        alsa->linkA(audio);
    }
    else
        audio=Link::create("InputAi");

    gain->linkA(encA);
    if(cfg["encv"].toMap()["lowLatency"].toBool())
        video->linkV(encV);
    else
        overlay->linkV(encV);

    if(cfg["encv2"].toMap()["lowLatency"].toBool())
        video->linkV(encV2);
    else
        overlay->linkV(encV2);

#if (defined HI3519A) || (defined HI3559A)
    if(enableAVS)
    {
        vi->linkV(AVS);
        viR->linkV(AVS);
        AVS->start();
    }
    else
    {
        video=vi;
    }
#else
    if(dei!=NULL)
        vi->linkV(dei);
#endif


    Channel::init();
}

void ChannelVI::updateConfig(QVariantMap cfg)
{
#if (defined HI3519A) || (defined HI3559A)
        if(enableAVS)
        {
            QVariantMap vd;
            vd["interface"]=cfg["interface"].toString()+"-L";
            vi->start(vd);

            vd["interface"]=cfg["interface"].toString()+"-R";
            viR->start(vd);
        }
        else
        {
            QVariantMap vd;
            vd["interface"]=cfg["interface"].toString();
            vi->start(vd);
        }
#else
        if(cfg["cap"].toMap()["deinterlace"].toBool() && dei!=NULL)
        {
            video=dei;
            dei->start();
            vi->unLinkV(overlay);
            dei->linkV(overlay);
        }
        else
        {
            video=vi;
            if(dei!=NULL)
            {
                dei->stop();
                dei->unLinkV(overlay);
            }
            vi->linkV(overlay);
        }

        QVariantMap vd;
        vd["interface"]=cfg["interface"].toString();
        vd["crop"]=cfg["cap"].toMap()["crop"].toMap();
        if(cfg["cap"].toMap().contains("rotate"))
            vd["rotate"]=cfg["cap"].toMap()["rotate"].toInt();
        if(cfg["cap"].toMap().contains("contrast"))
            vd["contrast"]=cfg["cap"].toMap()["contrast"].toInt();
        vi->start(vd);
#endif

    if(cfg["enable"].toBool())
    {
        QVariantMap ad;
        if(cfg.contains("alsa"))
        {
            ad["interface"]=cfg["interface"].toString();
            alsa->start(ad);
        }
        else
        {
            ad["interface"]=cfg["interface"].toString();
#if defined HI3516E || defined HI3516CV610
            ad["type"]=(cfg["enca"].toMap()["audioSrc"].toString()=="line")?"codec":"i2s";
            audio->linkA(gain);
            QVariantMap dataAo;
            dataAo["interface"]="Line-Out";
            if(ad["type"].toString()=="codec")
                ipcAo->start(dataAo);
            else
                ipcAo->stop();
            audio->linkA(ipcAo);

#endif
            if(cfg["enca"].toMap().contains("audioTrack"))
                ad["track"] = cfg["enca"].toMap()["audioTrack"].toInt();
            audio->start(ad);

        }

        if(cfg["enca"].toMap()["codec"].toString()!="close")
        {
            QVariantMap dd=cfg["enca"].toMap();
            dd.remove("audioSrc");
            dd.remove("audioTrack");
            encA->start(dd);
        }
        else
            encA->stop();

        if(cfg["encv"].toMap()["codec"].toString()!="close")
        {
            QVariantMap dataEncv=cfg["encv"].toMap();
#if (defined HI3519A) ||  (defined HI3559A)
            dataEncv["scaleUp"]=true;
#endif
            dataEncv["ntsc"]=cfg["cap"].toMap()["ntsc"].toBool();
            encV->start(dataEncv);
        }
        else
            encV->stop();

        if(cfg["enable2"].toBool()  && cfg["encv2"].toMap()["codec"].toString()!="close")
            encV2->start(cfg["encv2"].toMap());
        else
            encV2->stop();

//        {
//            QVariantMap dataSnap;
//            dataSnap["width"]=cfg["encv"].toMap()["width"].toInt();
//            dataSnap["height"]=cfg["encv"].toMap()["height"].toInt();
//            snap->setData(dataSnap);
//        }


    }
    else
    {
        encA->stop();
        encV->stop();
    }

    Channel::updateConfig(cfg);
}
