#ifndef PUSH_H
#define PUSH_H

#include <QObject>
#include "Link.h"
#include <QDateTime>
#include <QTimer>

class Push : public QObject
{
    Q_OBJECT
public:
    explicit Push(QObject *parent = 0);
    void init();
private:
    QVariantMap config;
    LinkObject *srcA;
    LinkObject *srcV;

    LinkObject *lastSrcA;
    LinkObject *lastSrcV;
    struct PushUrl
    {
        QString type;
        QString path;
        LinkObject *mux;
        bool enable;
        QString flvflags;
        int count;
        bool push;
    };
    QList<PushUrl*> urlList;
    bool bPushing;
    LinkObject *preview;

    QTimer *pushTimer;
    int pushCount;
    QString web;


signals:
    void newEvent(QString type, QVariant msg);

public slots:
    bool start();
    bool stop();
    QVariantMap getState();
    bool update(QString json);
    void onPushCountTimer();
};
extern Push *GPush;
#endif // PUSH_H
