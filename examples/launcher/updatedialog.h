#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <updater.h>

namespace Ui {
class UpdateDialog;
}

class RemoteUpdate;
class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    UpdateDialog(QString processId, QString currentVersionPath, QString uptodateVersionPath);
    ~UpdateDialog();
public slots:
    void update();

private slots:
    void copyProgress(qint64 current, qint64 total);
    void copyWarning(const Warning &msg);
    void copyFinished();

private:
    QString processId, currentVersionPath, uptodateVersionPath;
    Updater m_launcherUpdate;
    QString m_exe;
    Ui::UpdateDialog *ui;
};

#endif // UPDATEDIALOG_H
