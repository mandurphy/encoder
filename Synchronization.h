#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include <QObject>
#include <QVariantList>

class Synchronization : public QObject
{
    Q_OBJECT
public:
    explicit Synchronization(QObject *parent = nullptr);
    void init();
signals:
public slots:
    bool update(QVariantList list);
};
extern Synchronization *GSync;
#endif // SYNCHRONIZATION_H
