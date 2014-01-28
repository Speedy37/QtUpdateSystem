#include <QString>

#ifndef EXCEPTION_H
#define EXCEPTION_H

class Exception
{
public:
    Exception()
    {
        this->errorType = 0;
    }

    Exception(int errorType, const QString &errorString)
    {
        this->errorType = errorType;
        this->errorString = errorString;
    }

    int errorType;
    QString errorString;
};
#endif // EXCEPTION_H
