#ifndef INTERCOM_H
#define INTERCOM_H

#include <QObject>
#include "Link.h"
#include <QTimer>

class Intercom : public QObject
{
    Q_OBJECT
public:
    explicit Intercom(QObject *parent = 0);
    void init();
    LinkObject *ai;
    LinkObject *ao;
    LinkObject *res1;
    LinkObject *res2;
    LinkObject *gain;
    LinkObject *intercom;
    LinkObject *server;
    LinkObject *vmix;
    LinkObject *tally;
    QVariant tallyInfo;
    QTimer timer;
signals:
    void newEvent(QString type, QVariant msg);
public slots:
    bool update(QVariantMap cfg);
    bool setTally(QVariantList list);
    QVariantMap getState();
    void onTimer();
    void onNewEvent(QString type,QVariant info);
};

extern Intercom *GIntercom;
#endif // INTERCOM_H
