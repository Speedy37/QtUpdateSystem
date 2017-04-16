#include "updatedialog.h"
#include "ui_updatedialog.h"
#include <QTimer>
#include <QDebug>

#if defined(Q_OS_WIN)
#  include <Windows.h>
#endif

UpdateDialog::UpdateDialog(QString processId, QString currentVersionPath, QString uptodateVersionPath) :
    QDialog(0),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);

    this->processId = processId;
    this->currentVersionPath = currentVersionPath;
    this->uptodateVersionPath = uptodateVersionPath;
    connect(&m_launcherUpdate, &Updater::copyFinished, this, &UpdateDialog::copyFinished);
    connect(&m_launcherUpdate, &Updater::copyProgress, this, &UpdateDialog::copyProgress);
    connect(&m_launcherUpdate, &Updater::warning, this, &UpdateDialog::copyWarning);
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::update()
{
#if defined(Q_OS_WIN)
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, processId.toULong());
    if(hProcess != NULL)
    {
        DWORD dwExitCode;
        if(!GetExitCodeProcess(hProcess, &dwExitCode) || dwExitCode == STILL_ACTIVE)
        {
            CloseHandle(hProcess);
            ui->updateLog->appendPlainText(tr("Process is still running, waiting..."));
            QTimer::singleShot(100, this, SLOT(update()));
            return;
        }
        CloseHandle(hProcess);
    }
    ui->updateLog->appendPlainText(tr("Process is not running, copy..."));
#else
#  warning "Auto update isn't supported, implement it"
#endif

    m_launcherUpdate.setLocalRepository(uptodateVersionPath);
    m_launcherUpdate.copy(currentVersionPath);
}

void UpdateDialog::copyProgress(qint64 current, qint64 total)
{
    ui->updateProgess->setValue((current*100)/total);
}

void UpdateDialog::copyWarning(const Warning &msg)
{
    ui->updateLog->appendPlainText(msg.operation().path + " : " + msg.message());
}

void UpdateDialog::copyFinished()
{
    QString exe = currentVersionPath + QStringLiteral("/Launcher.exe");
    QProcess::startDetached(exe, QStringList() << "updated", currentVersionPath);
    close();
}
