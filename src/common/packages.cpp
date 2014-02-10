#include "packages.h"
#include "package.h"
#include "jsonutil.h"

#include <qtlog.h>
#include <QMap>
#include <QTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

void Packages::addPackage(const Package &package)
{
    m_packages.append(package);
}

void Packages::loadPackages(const QJsonObject & object)
{
    const QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        loadPackages1(object);
    else
        throw(QObject::tr("Unsupported version %1").arg(version));
}

QJsonObject Packages::toJsonObject() const
{
    QJsonObject object;
    QJsonArray packages;

    for(int i = 0; i < m_packages.size(); ++i)
    {
        packages.append(m_packages[i].toJsonObject());
    }

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("packages"), packages);

    return object;
}

void Packages::loadPackages1(const QJsonObject object)
{
    const QJsonArray packages = JsonUtil::asArray(object, QStringLiteral("packages"));

    m_packages.resize(packages.size());

    for(int i = 0; i < packages.size(); ++i)
    {
        Package & package = m_packages[i];
        package.fromJsonObject(JsonUtil::asObject(packages[i]));
    }
}

class Edge;

class Node
{
private:
    Q_DISABLE_COPY(Node)
public:
    Node(const QString &name)
    {
        this->name = name;
        parcouru = LLONG_MAX;
        previousNode = NULL;
        previousPackage = NULL;
        done = false;
    }

    QString name;
    qint64 parcouru;
    QVector<Edge*> childs;
    Node * previousNode;
    Package * previousPackage;
    bool done;
};

class Edge
{
private:
    Q_DISABLE_COPY(Edge)
public:
    Edge() {}
    Node * to;
    Package* package;
};

QVector<Package> Packages::findBestPath(const QString &from, const QString &to)
{
    if(from == to)
        return QVector<Package>();

    QTime t;
    t.start();

    // Build the graph
    QMap<QString, Node*> nodes;
    Node * startNode = new Node(from);
    Node * endNode = new Node(to);
    Edge * edges = new Edge[m_packages.length()];
    {
        Edge * edge;
        QMap<QString, Node*>::iterator fromNodeIt, toNodeIt;
        Package * package;
        nodes.insert(from, startNode);
        nodes.insert(to, endNode);
        for(int i = 0; i < m_packages.length(); ++i)
        {
            package = &(m_packages[i]);
            edge = &(edges[i]);

            fromNodeIt = nodes.find(package->from.isEmpty() ? from : package->from);
            if(fromNodeIt == nodes.end())
                fromNodeIt = nodes.insert(package->from, new Node(package->from));
            fromNodeIt.value()->childs << edge;

            toNodeIt = nodes.find(package->to);
            if(toNodeIt == nodes.end())
                toNodeIt = nodes.insert(package->to, new Node(package->to));

            edge->to = toNodeIt.value();
            edge->package = package;
        }
    }

    LOG_INFO(QObject::tr("Graph built in %1 ms").arg(t.restart()));

    QVector<Package> path;
    Node * node = startNode;
    qint64 bestParcouru;
    node->parcouru = 0;

    do
    {
        if(node == endNode)
            break;

        foreach(Edge *edge, node->childs)
        {
            qint64 parcouru = node->parcouru + edge->package->size;
            if(edge->to->parcouru > parcouru)
            {
                edge->to->parcouru = parcouru;
                edge->to->previousNode = node;
                edge->to->previousPackage = edge->package;
            }
        }

        bestParcouru = LLONG_MAX;
        node = NULL;
        foreach(Node * n, nodes)
        {
            if(!n->done && n->parcouru < bestParcouru)
            {
                bestParcouru = n->parcouru;
                node = n;
            }
        }
    }
    while(node != NULL);

    if(node == endNode)
    {
        while(node != startNode)
        {
            Q_ASSERT(node != NULL);
            path.prepend(*node->previousPackage);
            node = node->previousNode;
        }
        LOG_INFO(QObject::tr("Best path found in %1 ms").arg(t.elapsed()));
    }
    else
    {
        LOG_WARN(QObject::tr("No path found from %1 to %2").arg(from, to));
    }

    delete[] edges;

    return path;
}
