#ifndef CHANNEL_H
#define CHANNEL_H

#include <QObject>
#include "Link.h"
#include <QMap>
#include <QTimer>
#include "Json.h"

struct Stream
{
    QString streamPath;
    QString streamSuffix;
    LinkObject *mux;
    LinkObject *udpServer;
    static LinkObject *httpServer;
    static LinkObject *rtspServer;
    static LinkObject *webrtcServer;

    Stream(QString type, QString suffix, QString chnId) : streamSuffix(suffix)
    {
        mux = nullptr;
        udpServer = nullptr;
        QString streamFormat;

        if (type != "ndi")
        {
            mux = Link::create("Mux");
            if (type == "ts")
            {
                udpServer = Link::create("TSUdp");
                mux->linkV(udpServer);
            }
        }
        else
        {
        #if !defined HI3516E && !defined HI3516CV610
            mux = Link::create("NDISend");
        #endif
        }

        if(httpServer==NULL)
        {
            httpServer=Link::create("TSHttp");
            QVariantMap httpData;
            httpData["TSBuffer"]=0;
            httpServer->start(httpData);
        }

        if(rtspServer==NULL)
        {
            rtspServer=Link::create("Rtsp");
            rtspServer->start();
        }

        if(webrtcServer==NULL)
        {
        #if !defined(HI3521D) && !defined (HI3531D)
            webrtcServer=Link::create("WebRTC");
            QVariantMap webrtcConf = Json::loadFile("/link/config/rproxy/webrtc.json").toMap();
            webrtcServer->start(webrtcConf);
        #endif
        }


        if (mux != nullptr)
        {
            if (type == "rtmp")
            {
                streamPath = "rtmp://127.0.0.1/live/" + streamSuffix;
                streamFormat = "flv";
            }
            else if (type == "push")
            {
                streamPath = "rtmp://127.0.0.1/live/test" + chnId;
                streamFormat = "flv";
            }
            else if (type == "hls")
            {
                streamPath = "/tmp/hls/" + streamSuffix + ".m3u8";
                streamFormat = "hls";
            }
            else if (type == "srt")
            {
                streamPath = "srt://:" + QString::number(9000) + "?mode=listener&latency=50";
                streamFormat = "mpegts";
            }
            else if (type == "ts" || type == "rtsp" || type == "webrtc")
            {
                streamPath = "mem://" + streamSuffix;
                streamFormat = (type == "ts") ? "mpegts" : type;
            }

            QVariantMap data;
            data["path"] = streamPath;
            data["format"] = streamFormat;
            mux->setData(data);
        }
    }
};


class Channel : public QObject
{
    Q_OBJECT
public:
    explicit Channel(QObject *parent = 0);
    virtual void init(QVariantMap cfg=QVariantMap());
    virtual void updateConfig(QVariantMap cfg);
    QString writeCom(const QString &com);
    QString doSnap(const int &mod = 0,const QVariantList &chnIds = QVariantList());
    QString type;
    bool enableAVS;    
    bool isSrcLine;
    int lastAId;
    int id;

    static LinkObject *lineIn;
    static LinkObject *lineOut;
    static LinkObject *alsa;
    static LinkObject *ndiRecv;
    QVariantMap data;
    LinkObject *audio;
    LinkObject *video;
    LinkObject *volume;
    LinkObject *encA;
    LinkObject *encV;
    LinkObject *gain;

    LinkObject *overlay;
    LinkObject *snap;
    bool enable;
    LinkObject *encV2;
    QMap<QString,Stream*> streamMap;
    QMap<QString,Stream*> streamMap_sub;

    QString chnName;
    QTimer *cd_pauseTimer = nullptr;
    QVariantList layList;
    QMap<QString,LinkObject*> formatMap;
    bool isRecord = false;
    QString startRecordTime = nullptr;
    int pauseTime = 0;

    int timerStrToInt(QString time);
    void startRecord(const QString &fileName,const QString &format = "mp4",QVariantMap fragment = QVariantMap());
    void stopRecord(const QString &format = "mp4");
    void recordPuase(const bool &pause = false);
signals:

public slots:
    void cdPauseTimeout();

};

#endif // CHANNEL_H
