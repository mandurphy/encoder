#include "LED.h"
#include <QFile>
#include "Json.h"
#include "Config.h"
#include "ChannelVI.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Record.h"
#include "Push.h"
#include "Intercom.h"

LED::LED(QObject *parent) : QObject(parent)
{
    fd=0;
    mode=ModeCut;
    tallyArbiter=NULL;
    dstR=0;
    dstG=0;
    dstB=0;
    lastR=1;
    lastG=1;
    lastB=1;
    connect(&timer,SIGNAL(timeout()),this,SLOT(onTimer()));

}

void LED::init()
{
    if(!QFile::exists(LEDCFGFILE))
        return;

    if(!QFile::exists("/dev/led_ctrl"))
        return;

    fd = open("/dev/led_ctrl", O_RDWR);
    if (fd < 0)
        return;

    devCfg=Json::loadFile(LEDDEVFILE).toMap();

    update(Json::loadFile(LEDCFGFILE).toMap());
}

bool LED::update(QVariantMap cfg)
{
    if(cfg.isEmpty())
        return false;

    config=cfg;

    if(config["enable"].toBool())
    {
        if(Config::chns[0]->type=="vi")
        {
            ChannelVI *chnVI=(ChannelVI *)Config::chns[0];
            connect(chnVI->vi,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
            curInfo=chnVI->vi->invoke("getReport");
        }

        if(GRecord!=NULL)
        {
            connect(GRecord,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
        }

        if(GPush!=NULL)
        {
            connect(GPush,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
        }

        if(GIntercom!=NULL)
        {
            connect(GIntercom,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
        }


        func=config["func"].toString();
        funcArgs=config["funcList"].toMap()[func].toMap();
        if(funcArgs["mode"].toString()=="cut")
            mode=ModeCut;
        else if(funcArgs["mode"].toString()=="breathe")
            mode=ModeBreathe;
        else if(funcArgs["mode"].toString()=="flick")
            mode=ModeFlick;
        else if(funcArgs["mode"].toString()=="slide")
            mode=ModeSlide;
        if(func=="tallyArbiter")
        {
            if(tallyArbiter==NULL)
            {
                tallyArbiter=Link::create("TallyArbiter");
                connect(tallyArbiter,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onNewEvent(QString,QVariant)));
            }
            tallyArbiter->start(funcArgs);
        }
        onNewInfo();
    }
    else
    {
        timer.stop();
        fillColor(0,0,0);
    }
    return true;
}

void LED::onNewEvent(QString type, QVariant info)
{
    if(func=="signal" && type=="signal")
    {
        curInfo=info;
        onNewInfo();
    }
    else if(func=="record" && type=="record")
    {
        curInfo=info;
        onNewInfo();
    }
    else if(func=="push" && type=="push")
    {
        curInfo=info;
        onNewInfo();
    }
    else if(func=="tally" && type=="TALLY")
    {
        curInfo=info;
        onNewInfo();
    }
    else if(func=="tallyArbiter" && type=="TA_color")
    {
        curInfo=info;
        onNewInfo();
    }
    else if(func=="tallyArbiter" && type=="TA_reassign")
    {
        QVariantMap cfg=Json::loadFile(LEDCFGFILE).toMap();
        QVariantMap flist=cfg["funcList"].toMap();
        QVariantMap taMap=flist["tallyArbiter"].toMap();
        taMap["deviceId"]=info.toMap()["deviceId"];
        taMap["deviceName"]=info.toMap()["deviceName"];
        flist["tallyArbiter"]=taMap;
        cfg["funcList"]=flist;
        Json::saveFile(cfg,LEDCFGFILE);
    }
}

void LED::onNewInfo()
{
    if(!config["enable"].toBool())
        return;
    if(func=="signal")
    {
        if(curInfo.toMap()["avalible"].toBool())
            setLED(1);
        else
            setLED(0);
    }
    else if(func=="record")
    {
        if(curInfo.toBool())
            setLED(1);
        else
            setLED(0);
    }
    else if(func=="push")
    {
        if(curInfo.toBool())
            setLED(1);
        else
            setLED(0);
    }
    else if(func=="tally")
    {
        int did=Json::loadFile("/link/config/intercom.json").toMap()["intercom"].toMap()["did"].toInt();
        QVariantList list=curInfo.toList();
        if(did<=0 || did>list.count())
            setLED(0);
        else
        {
            int c=list[did-1].toInt();
            if(c<=2)
                setLED(c);
            else
                setLED(0);
        }

    }
    else if(func=="tallyArbiter")
    {

        if(curInfo.toString().isEmpty())
            return;
        QByteArray color=QByteArray::fromHex(curInfo.toString().mid(1).toLatin1());
        double brightness=config["brightness"].toDouble();
        dstR=color[0]*brightness;
        dstG=color[1]*brightness;
        dstB=color[2]*brightness;
        fillColor(dstR,dstG,dstB);
    }

}

void LED::setLED(int state)
{
    QVariantList color=funcArgs["color"].toList()[state].toList();
    double brightness=config["brightness"].toDouble();
    dstR=color[0].toInt()*brightness;
    dstG=color[1].toInt()*brightness;
    dstB=color[2].toInt()*brightness;

    if(mode==ModeCut)
    {
        fillColor(dstR,dstG,dstB);
    }
    else if(mode==ModeBreathe || mode==ModeSlide)
    {
        if(timer.isActive() && timer.interval()!=20)
            timer.stop();
        if(!timer.isActive())
            timer.start(20);
    }
    else if(mode==ModeFlick)
    {
        if(timer.isActive() && timer.interval()!=1000)
            timer.stop();
        if(!timer.isActive())
            timer.start(1000);
    }


}

void LED::fillColor(uchar r, uchar g, uchar b)
{
    int count=devCfg["count"].toInt();
    uchar buf[count*3];
    for(int i=0;i<count;i++)
    {
        buf[i*3]=g;
        buf[i*3+1]=r;
        buf[i*3+2]=b;
    }

    write(fd, (void *)buf, count*3);
}

void LED::onTimer()
{
    if(mode==ModeSlide)
    {
        static int c=0;
        QVariantList list=devCfg["index"].toList();
        int count=devCfg["count"].toInt();
        static uchar *buf=NULL;
        if(buf==NULL)
        {
            buf=new uchar[count*3];
            memset(buf,0,count*3);
        }
        int cnt=list.count();
        int a=40/cnt;

        c++;
        if(c>40)
        {
            c=0;
            timer.stop();
            return;
        }

        int m=c/a;
        int n=c%a;
        for(int i=0;i<m;i++)
        {
            QVariantList ll=list[i].toList();
            for(int j=0;j<ll.count();j++)
            {
                int id=ll[j].toInt();
                buf[id*3+0]=dstG;
                buf[id*3+1]=dstR;
                buf[id*3+2]=dstB;
            }
        }

        if(m<cnt)
        {

            QVariantList ll=list[m].toList();
            for(int j=0;j<ll.count();j++)
            {
                int id=ll[j].toInt();
                buf[id*3+0]=dstG*n/a;
                buf[id*3+1]=dstR*n/a;
                buf[id*3+2]=dstB*n/a;
            }

        }


        write(fd, (void *)buf, count*3);

    }
    else if(mode==ModeBreathe)
    {
        static int c=0;
        static int k=1;

        c+=k;
        if(c>50)
        {
            c=50;
            k=-1;
        }
        else if(c<0)
        {
            c=0;
            k=1;
        }
        uchar r=(int)dstR*c/50;
        uchar g=(int)dstG*c/50;
        uchar b=(int)dstB*c/50;
        if(r!=lastR || g!=lastG && b!=lastB)
            fillColor(r,g,b);
        lastR=r;
        lastG=g;
        lastB=b;
    }
    else if(mode==ModeFlick)
    {
        static bool show=false;
        show=!show;
        if(show)
            fillColor(dstR,dstG,dstB);
        else
            fillColor(0,0,0);
    }

}

QVariantList LED::getTADevices()
{
    QVariantList ret;
    if(tallyArbiter==NULL)
        return ret;
    ret=tallyArbiter->invoke("getDevices").toList();
    return ret;
}
