#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include "../errors/warning.h"
#include "localrepository.h"
#include <QThread>
#include <QString>
#include <QSet>


class CopyThread : public QThread
{
    Q_OBJECT
public:
    CopyThread(const LocalRepository &sourceRepository, const QString &destinationDir, QObject *parent = 0);

signals:
    void warning(const Warning &warning);
    void progression(int copiedFileCount, int totalFileCount);
    void copyFinished(const QString &errorString);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    LocalRepository m_sourceRepository;
    QString m_destinationDir;

};

#endif // COPYTHREAD_H
