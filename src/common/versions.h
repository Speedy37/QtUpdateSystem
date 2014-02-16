#ifndef VERSIONS_H
#define VERSIONS_H

#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "version.h"

class Versions : public QVector<Version>
{
public:
    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;

    void fromJsonArrayV1(const QJsonArray &versions);
    QJsonArray toJsonArrayV1() const;
};

#endif // VERSIONS_H
