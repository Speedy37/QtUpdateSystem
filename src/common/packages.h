#ifndef PACKAGES_H
#define PACKAGES_H

#include <QVector>
#include <QJsonObject>
#include "package.h"

class Packages : public QVector<Package>
{
public:
    QVector<Package> findBestPath(const QString &from, const QString &to);

    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;

    void fromJsonArrayV1(const QJsonArray &packages);
    QJsonArray toJsonArrayV1() const;
};

#endif // PACKAGES_H
