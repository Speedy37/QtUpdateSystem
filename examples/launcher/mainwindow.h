#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "borderlesswindow.h"
#include <updater.h>

class LoginView;

class MainWindow : public QuickBorderlessView
{
    Q_OBJECT
    Q_ENUMS(ConfigState)
public:
    enum ConfigState
    {
        Init,
        CheckingForLauncherUpdates,
        CheckingForLauncherUpdatesFailed,
        UpdatingLauncher,
        UpdatingLauncherFailed,
        LauncherReady,
        CheckingForGameUpdates,
        CheckingForGameUpdatesFailed,
        UpdatingGame,
        UpdatingGameFailed,
        Ready
    };

    void setView(LoginView * view) { m_loginview = view; }

//>>> QML Properties
private:
    Q_PROPERTY(float checkProgression MEMBER m_checkProgression NOTIFY checkProgressionChanged)
    Q_PROPERTY(float downloadProgression MEMBER m_downloadProgression NOTIFY downloadProgressionChanged)
    Q_PROPERTY(float applyProgression MEMBER m_applyProgression NOTIFY applyProgressionChanged)
    int m_checkProgression, m_downloadProgression, m_applyProgression;

    Q_PROPERTY(QString progressLabel MEMBER m_progressLabel NOTIFY progressLabelChanged)
    Q_PROPERTY(QString gamelabel MEMBER m_gamelabel NOTIFY gamelabelChanged)
    QString m_progressLabel, m_gamelabel;

    Q_PROPERTY(QString launcherVersion MEMBER m_launcherVersion NOTIFY launcherVersionChanged)
    Q_PROPERTY(QString gameVersion MEMBER m_gameVersion NOTIFY gameVersionChanged)
    QString m_launcherVersion, m_gameVersion;

    QString m_gamePath;

signals:
    void checkProgressionChanged();
    void downloadProgressionChanged();
    void applyProgressionChanged();
    void gameDx11Changed();
    void gameLogChanged();
    void game64bitChanged();
    void progressLabelChanged();
    void gamelabelChanged();
    void usernameChanged();
    void passwordChanged();
    void gamePathChanged();
    void launcherVersionChanged();
    void gameVersionChanged();

public slots:
    void onGameClicked();
    void load();
    void save();
//<<< QML Properties

public:
    MainWindow(QWindow *parent = 0);
    virtual	~MainWindow();
    ConfigState state() const;

private slots:
    void onUpdateCheckProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onUpdateApplyProgress(qint64 current, qint64 total);
    void onUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    void launcherCheckForUpdates();
    void launcherCheckForUpdatesFinished(bool success);
    void launcherCopyFinished();
    void launcherUpdateFinished(bool success);

    void gameCheckForUpdates();
    void gameCheckForUpdatesFinished(bool success);
    void gameUpdateFinished(bool success);

private:
    void setState(ConfigState state);
    QString path(const QString &base, const QString &subdir, bool autoCreate = false);
    ConfigState m_state;
    Updater m_updater_game, m_updater_launcher;
    LoginView *m_loginview;
};

inline MainWindow::ConfigState MainWindow::state() const
{
    return m_state;
}

#endif // MAINWINDOW_H
