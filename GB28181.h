#ifndef GB28181_H
#define GB28181_H

#include <QObject>
#include <QMap>
#include "Link.h"

class GB28181 : public QObject
{
    Q_OBJECT
public:
    explicit GB28181(QObject *parent = nullptr);
    void init();
    bool loadConfig(QVariantMap config);
private:
    QVariantMap config;
    LinkObject *svr;
    struct GbChannel
    {
        LinkObject *mux;
        QVariantMap cfg;
        GbChannel() {mux=NULL;}
    };
    QMap<int,GbChannel*> chnMap;
signals:
public slots:
    bool update(QString json);

};
extern GB28181 *Ggb28181;
#endif // GB28181_H
