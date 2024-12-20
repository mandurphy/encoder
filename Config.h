#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <Channel.h>
#include <QList>
#include <QApplication>

#define CFGPATH (QApplication::arguments().contains("-c") && QApplication::arguments().indexOf("-c") + 1 < QApplication::arguments().size() ? QApplication::arguments()[QApplication::arguments().indexOf("-c") + 1] : "/link/config/config.json")
#define GRPPATH "/link/config/group.json"
#define HARDWAREPATH "/link/config/hardware.json"
#define NETPATH "/link/config/net.json"
#define RECPATH "/link/config/record.json"
#define BOARDPATH "/link/config/board.json"
#define DEFAULT_BOARDPATH "/link/config/default/board.json"
#define BUFFERPATH "/link/config/videoBuffer.json"
#define OVERLAYPATH "/link/config/auto/overlay.json"
#define ROIPATH "/link/config/auto/roi.json"

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = 0);
    static void loadConfig(QString path);
    static QList<Channel*> chns;
    static QVariantList overlayList;
    static QVariantList roiList;
    static Channel* findChannelById(int id);
    static void loadAutoConfig();
signals:

public slots:

};

#endif // CONFIG_H
