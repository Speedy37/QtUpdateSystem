#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include "localrepository.h"
#include <QThread>
#include <QString>
#include <QSet>


class CopyThread : public QThread
{
    Q_OBJECT
public:
    CopyThread(const LocalRepository *sourceRepository, const QString &destinationDir, QObject *parent = 0);

signals:
    void progression(int copiedFileCount, int totalFileCount);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    const LocalRepository *m_sourceRepository;
    QString m_destinationDir;

};

#endif // COPYTHREAD_H
