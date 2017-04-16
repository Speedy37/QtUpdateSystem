#include "mainwindow.h"
#include <QDebug>
#include <QQuickItem>
#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QQmlContext>
#include <QFileDialog>

#ifdef QT_DEBUG
#  include <QTimer>
#endif

const QString IniFilename = QStringLiteral("Launcher.ini");

const QString Channel = QStringLiteral("channel");
const QString GameUpdateUrl = QStringLiteral("https://example.com/releases/game/%1/");
const QString LauncherUpdateUrl = QStringLiteral("https://example.com/releases/launcher/%1/");

MainWindow::MainWindow(QWindow *parent) : QuickBorderlessView(parent)
{
    setTitle(tr("Launcher"));
    setState(Init);

    m_checkProgression = 0;
    m_downloadProgression = 0;
    m_applyProgression = 0;
    m_updater_game.setRemoteRepository(GameUpdateUrl.arg(Channel));
    m_updater_launcher.setRemoteRepository(LauncherUpdateUrl.arg(Channel));
    m_updater_launcher.setLocalRepository(QCoreApplication::applicationDirPath());
    m_launcherVersion = m_updater_launcher.localRevision();
    m_gameVersion = tr("checking ...", "game version");

    connect(&m_updater_launcher, &Updater::checkForUpdatesFinished, this, &MainWindow::launcherCheckForUpdatesFinished);
    connect(&m_updater_launcher, &Updater::copyFinished, this, &MainWindow::launcherCopyFinished);
    connect(&m_updater_launcher, &Updater::updateCheckProgress, this, &MainWindow::onUpdateCheckProgress);
    connect(&m_updater_launcher, &Updater::updateDownloadProgress, this, &MainWindow::onUpdateDownloadProgress);
    connect(&m_updater_launcher, &Updater::updateApplyProgress, this, &MainWindow::onUpdateApplyProgress);
    connect(&m_updater_launcher, &Updater::updateFinished, this, &MainWindow::launcherUpdateFinished);
    connect(&m_updater_launcher, &Updater::warning, [](const Warning &msg) {
        qDebug() << "launcherUpdate" << msg.operation() << msg.typeString() << msg.message();
    });
    connect(&m_updater_game, &Updater::checkForUpdatesFinished, this, &MainWindow::gameCheckForUpdatesFinished);
    connect(&m_updater_game, &Updater::updateCheckProgress, this, &MainWindow::onUpdateCheckProgress);
    connect(&m_updater_game, &Updater::updateDownloadProgress, this, &MainWindow::onUpdateDownloadProgress);
    connect(&m_updater_game, &Updater::updateApplyProgress, this, &MainWindow::onUpdateApplyProgress);
    connect(&m_updater_game, &Updater::updateFinished, this, &MainWindow::gameUpdateFinished);
    connect(&m_updater_game, &Updater::warning, [](const Warning &msg) {
        qDebug() << "gameUpdate" << msg.operation() << msg.typeString() << msg.message();
    });

    load();
    rootContext()->setContextProperty("window", this);
    setSource(QUrl("qrc:/main.qml"));
    launcherCheckForUpdates();
}

MainWindow::~MainWindow()
{
    save();
}

void MainWindow::load()
{
    QSettings opts(IniFilename, QSettings::IniFormat);

    opts.beginGroup("Config");
    opts.endGroup();
}

void MainWindow::save()
{
    QSettings opts(IniFilename, QSettings::IniFormat);

    opts.beginGroup("Config");
    opts.endGroup();
}

void MainWindow::onGameClicked()
{
    if(state() == Ready)
    {
        /*if(!QProcess::startDetached(exe, QStringList() << m_serverip, dir))
        {
            QMessageBox::critical(0, tr("Starting Game"),
                                  tr("Unable to start the game."));
        }*/
    }
    else if(state() > Init)
    {
        return; // Something is in progress
    }
    else
    {
        if(!QFileInfo(m_gamePath).isDir())
        {
            m_gamePath = QFileDialog::getExistingDirectory(0, tr("Select the game install directory"), m_gamePath);
            if(m_gamePath.isEmpty() || !QFileInfo(m_gamePath).isDir())
                return;
        }
        launcherCheckForUpdates();
    }
}

void MainWindow::launcherCheckForUpdates()
{
    ConfigState oldState = state();
    try
    {
        setState(CheckingForLauncherUpdates);
        m_updater_launcher.setLocalRepository(QCoreApplication::applicationDirPath());
        m_launcherVersion = m_updater_launcher.localRevision();
        emit launcherVersionChanged();
        m_updater_launcher.setTmpDirectory(path(QCoreApplication::applicationDirPath(), QStringLiteral("Tmp"), true));
        //m_updater_launcher.setCredentials(m_username, m_password);
        m_updater_launcher.checkForUpdates();
    }
    catch(const QString &)
    {
        setState(oldState);
    }
}

void MainWindow::launcherCheckForUpdatesFinished(bool success)
{
    if(!success)
    {
        qWarning() << m_updater_launcher.errorString();
        setState(CheckingForLauncherUpdatesFailed);
        QMessageBox::critical(nullptr, m_progressLabel, m_updater_launcher.errorString());
    }
    else if(m_updater_launcher.isUpdateAvailable())
    {
        setState(UpdatingLauncher);
        m_updater_launcher.copy(path(QCoreApplication::applicationDirPath(), QStringLiteral("TmpCopy"), true));
    }
    else
    {
        gameCheckForUpdates();
    }
}

void MainWindow::launcherCopyFinished()
{
    m_updater_launcher.setLocalRepository(path(QCoreApplication::applicationDirPath(), QStringLiteral("TmpCopy"), true));
    m_updater_launcher.update();
}

void MainWindow::launcherUpdateFinished(bool success)
{
    if(!success)
    {
        qWarning() << m_updater_launcher.errorString();
    }
    else if(m_updater_launcher.localRevision() != m_launcherVersion)
    {
        m_launcherVersion = m_updater_launcher.localRevision();
        emit launcherVersionChanged();
        QMessageBox::information(0,
                                 tr("Launcher update"),
                                 tr("The application will now restart and apply updates.\n"
                                    "It's should be very fast"));
        QString exe, directory;
#ifdef QT_DEBUG
        directory = path(QCoreApplication::applicationDirPath(), QStringLiteral("TmpCopyTest"));
#else
        directory = path(QCoreApplication::applicationDirPath(), QStringLiteral("TmpCopy"));
#endif
        QString processId;
#if defined(Q_OS_WIN)
        exe = directory + QStringLiteral("/Launcher.exe");
        processId = QString::number(GetCurrentProcessId());
#else
#  warning "Auto update isn't supported, implement it"
#endif
        if(!QProcess::startDetached(exe,
                                    QStringList() << "update" << processId << QCoreApplication::applicationDirPath(),
                                    directory))
        {
            QMessageBox::critical(0, tr("Launcher update"),
                                  tr("Unable to restart and apply updates\n"
                                     "Tell the admin about that issue."));
        }

#ifdef QT_DEBUG
        QTimer::singleShot(1000, this, SLOT(close()));
#else
        close();
#endif
    }
}

void MainWindow::gameCheckForUpdates()
{
    ConfigState oldState = state();
    try
    {
        setState(CheckingForGameUpdates);
        m_updater_game.setLocalRepository(path(m_gamePath, "", true));
        m_updater_game.setTmpDirectory(path(m_gamePath, QStringLiteral("UpdateTmp"), true));
        //m_updater_game.setCredentials(m_username, m_password);
        m_updater_game.checkForUpdates();
    }
    catch(const QString &msg)
    {
        qWarning() << msg;
        setState(oldState);
    }
}

void MainWindow::gameCheckForUpdatesFinished(bool success)
{
    if(!success)
    {
        qWarning() << m_updater_game.errorString();
    }
    else if(m_updater_game.isUpdateAvailable())
    {
        setState(UpdatingGame);
        m_updater_game.update();
    }
    else
    {
        setState(Ready);
    }
}

void MainWindow::gameUpdateFinished(bool success)
{
    if(!success)
    {
        qWarning() << m_updater_game.errorString();
    }
    else
    {
        setState(Ready);
    }
}

float progression(qint64 bytesReceived, qint64 bytesTotal)
{
    if(bytesTotal == 0 || bytesReceived > bytesTotal)
        return 1.0;
    if(bytesReceived < 0)
        return 0.0;
    return float(bytesReceived)/float(bytesTotal);
}

void MainWindow::onUpdateCheckProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_checkProgression = progression(bytesReceived, bytesTotal);
    emit checkProgressionChanged();
}

void MainWindow::onUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_downloadProgression = progression(bytesReceived, bytesTotal);
    emit downloadProgressionChanged();
}

void MainWindow::onUpdateApplyProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_applyProgression = progression(bytesReceived, bytesTotal);
    emit applyProgressionChanged();
}

QString MainWindow::path(const QString &base, const QString &subdir, bool autoCreate)
{
    if(base.isEmpty())
        throw tr("Path is empty");

    QDir dir(base);
    if(!dir.exists())
        throw tr("Path doesn't exists");

    if(!subdir.isEmpty() && !dir.cd(subdir))
    {
        if(!autoCreate)
            throw tr("Subdir %1 dir doesn't exists").arg(subdir);
        if(!dir.mkdir(subdir))
            throw tr("Unable to create subdir %1").arg(subdir);
        if(!dir.cd(subdir))
            throw tr("Subdir %1 dir doesn't exists").arg(subdir);
    }

    return dir.absolutePath();
}

void MainWindow::setState(MainWindow::ConfigState state)
{
    m_state = state;
    switch (state)
    {
    case Init:
        m_progressLabel = tr("Configuration is valid");
        break;
    case CheckingForLauncherUpdatesFailed:
        m_progressLabel = tr("Checking for launcher updates failed");
        break;
    case CheckingForLauncherUpdates:
        m_progressLabel = tr("Checking for launcher updates...");
        break;
    case UpdatingLauncher:
        m_progressLabel = tr("Updating launcher...");
        break;
    case CheckingForGameUpdates:
        m_progressLabel = tr("Checking for game updates...");
        break;
    case CheckingForGameUpdatesFailed:
        m_progressLabel = tr("Checking for game updates failed");
        break;
    case UpdatingGame:
        m_progressLabel = tr("Updating...");
        break;
    case Ready:
        m_checkProgression = 1;
        m_downloadProgression = 1;
        m_applyProgression = 1;
        emit checkProgressionChanged();
        emit downloadProgressionChanged();
        emit applyProgressionChanged();
        m_progressLabel = tr("Ready");
        break;
    }
    emit gamelabelChanged();
    emit progressLabelChanged();
}
