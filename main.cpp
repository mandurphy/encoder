#include <QApplication>
#include <QTextCodec>
#include <QFile>
#include "Link.h"
#include "Config.h"
#include "RPC.h"
#include "Json.h"
#include "Version.h"
#include "Record.h"
#include "Push.h"
#include "UART.h"
#include "Intercom.h"
#include "GB28181.h"
#include "Synchronization.h"
#include "oled/Oled.h"
#include <QFontDatabase>
#include "LED.h"
#include "Capture.h"

RPC *GRPC;
Record *GRecord;
Push *GPush;
UART *GUart;
Intercom *GIntercom;
GB28181 *Ggb28181;
Synchronization *GSync;
OLED *GOLED;
LED *GLED;
Capture *GCapture;

bool setVideoBuffer(int count)
{
    bool ret = true;
    QVariantMap videoBuffer = Json::loadFile(BUFFERPATH).toMap();
    if(videoBuffer.contains("used"))
    {
        QString current = videoBuffer["used"].toString();
        if(videoBuffer.contains(current))
        {
            QVariantMap curBuffer = videoBuffer[current].toMap();
            QVariantMap board = Json::loadFile(BOARDPATH).toMap();
            if(board.isEmpty())
                board = Json::loadFile(DEFAULT_BOARDPATH).toMap();
            board["videoBuffer"] = curBuffer;
            Json::saveFile(board,BOARDPATH);
            system("sync");
        }
    }

    if(count > 2)
        return false;

    QVariantMap nboard = Json::loadFile(BOARDPATH).toMap();
    if(nboard.isEmpty())
    {
        count++;
        ret = setVideoBuffer(count);
    }
    return ret;
}

bool hadOledExec()
{
    return QFile::exists("/link/bin/OLED");
}

int main(int argc, char *argv[])
{
    argv[argc++]="-platform";
    argv[argc++]="offscreen";

    QApplication a(argc, argv);
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    qDebug("start");

    if(!setVideoBuffer(0) || !Link::init())
        return 0;

    QString ver;
    ver=ver.sprintf("%s build %s_%d",VERSION_VER,VERSION_DATE,VERSION_BUILD);

    QVariantMap version=Json::loadFile("/link/config/version.json").toMap();
        version["app"]=ver;
        version["sdk"]=Link::getVersion()["version"].toString()
                +" build "+Link::getVersion()["date"].toString()
                +"_"+Link::getVersion()["build"].toString();
    Json::saveFile(version,"/link/config/version.json");

    GRPC=new RPC();
    GRPC->startNTP();

    Config::loadConfig(CFGPATH);
    Config::loadAutoConfig();

    GRecord=new Record();
    GRecord->init();

    GPush=new Push();
    GPush->init();

#ifndef HI3516CV610
    GUart=new UART();
    GUart->init();

    Ggb28181=new GB28181();
    Ggb28181->init();

    GSync=new Synchronization();
    GSync->init();

    GIntercom=new Intercom();
    GIntercom->init();
#endif

    GCapture=new Capture();
    GCapture->init();

    if(!hadOledExec())
    {
        GOLED=new OLED();
        GOLED->init();
    }

    GLED=new LED();
    GLED->init();

    GRPC->init();
    return a.exec();
}

