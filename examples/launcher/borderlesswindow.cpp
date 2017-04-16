#include "borderlesswindow.h"
#include <QQuickItem>

#ifdef Q_OS_WIN
#  include <Windows.h>
#  include <windowsx.h>
#endif

QuickBorderlessView::QuickBorderlessView(QWindow *parent) : QQuickView(parent)
{
   setFlags(Qt::FramelessWindowHint | Qt::Window);
   setColor(QColor(Qt::transparent));
}

QuickBorderlessView::~QuickBorderlessView()
{

}

#ifdef Q_OS_WIN
bool QuickBorderlessView::nativeEvent(const QByteArray &, void *message, long *result)
{
    MSG * msg = (MSG *)message;
    if(msg->message == WM_NCHITTEST)
    {
        LRESULT r = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        if(r == HTCLIENT && GetAsyncKeyState(MK_LBUTTON) < 0)
        {
            int xPos = GET_X_LPARAM(msg->lParam) - geometry().x();
            int yPos = GET_Y_LPARAM(msg->lParam) - geometry().y();
            QQuickItem *item = rootObject()->childAt(xPos, yPos);
            if(item != 0 && item->objectName() == "caption" && item->childAt(xPos, yPos) == 0)
            {
                *result = HTCAPTION;
                return true;
            }
        }
    }

    return false;
}
#endif
