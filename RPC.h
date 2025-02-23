#ifndef RPC_H
#define RPC_H

#include <QObject>
#include <jcon/json_rpc_tcp_server.h>
#include "Link.h"
#include "Group.h"
#include <QProcess>
#include <QTimer>

class RPC : public QObject
{
    Q_OBJECT
public:
    explicit RPC(QObject *parent = 0);
    void startNTP();
    void init();
    QString writeCom(const QString &com);
private:
    Group *group;
    jcon::JsonRpcTcpServer *rpcServer;
    LinkObject *device;
    QTimer timerSyncRTC;
signals:

public slots:
    bool update(QString json);
    bool updateOverlay(QString json);
    bool updateRoi(QString json);
    QVariantList snap(const QString &json = 0);
    QVariantMap getSysState();
    QVariantList getInputState();
    QVariantMap getNetState();
    QVariantList getEPG();
    QVariantList getPlayList();
    QVariantList getVolume();
    QVariantMap getPlayPosition();
    bool play(int index,int time);
    QVariantList getPushSpeed();
    QString getSN();
    QVariantList getNDIList();
    bool setNetDhcp(const bool &dhcp = true);
    bool setTrans(QString json);
    bool updateColorKey();
    bool pauseColorKey(const bool &pause = true);
    void onTimerSyncRTC();
    QString getChip();
    bool addOutputEdid(const QString &edidName);
    bool reloadConf();
};
extern RPC *GRPC;
#endif // RPC_H
