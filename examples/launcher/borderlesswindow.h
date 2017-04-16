#ifndef BORDERLESSWINDOW_H
#define BORDERLESSWINDOW_H

#include <QQuickView>

class QuickBorderlessView : public QQuickView
{
    Q_OBJECT

public:
    QuickBorderlessView(QWindow *parent = 0);
    virtual	~QuickBorderlessView();

#ifdef Q_OS_WIN
protected:
    bool nativeEvent(const QByteArray &, void *message, long *result);
#endif
};

#endif // BORDERLESSWINDOW_H
