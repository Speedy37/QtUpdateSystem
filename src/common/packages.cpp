#include "packages.h"
#include "jsonutil.h"

#include <qtlog.h>
#include <QMap>
#include <QTime>

static const QString Packages::FileName = QStringLiteral("packages");

void Packages::addPackage(const Package &package)
{
    m_packages.append(package);
}

void Packages::fromJsonObject(const QJsonObject & object)
{
    const QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        fromJsonArrayV1(JsonUtil::asArray(object, QStringLiteral("packages")));
    else
        throw(QObject::tr("Unsupported version %1").arg(version));
}

QJsonObject Packages::toJsonObject() const
{
    QJsonObject object;

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("packages"), toJsonArrayV1());

    return object;
}

void Packages::fromJsonArrayV1(const QJsonArray &packages)
{
    m_packages.resize(packages.size());

    for(int i = 0; i < packages.size(); ++i)
    {
        Package & package = m_packages[i];
        package.fromJsonObjectV1(JsonUtil::asObject(packages[i]));
    }
}

QJsonArray Packages::toJsonArrayV1() const
{
    QJsonArray packages;

    for(int i = 0; i < m_packages.size(); ++i)
    {
        packages.append(m_packages[i].toJsonObjectV1());
    }

    return packages;
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
        previousNode = nullptr;
        previousPackage = nullptr;
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
        node = nullptr;
        foreach(Node * n, nodes)
        {
            if(!n->done && n->parcouru < bestParcouru)
            {
                bestParcouru = n->parcouru;
                node = n;
            }
        }
    }
    while(node != nullptr);

    if(node == endNode)
    {
        while(node != startNode)
        {
            Q_ASSERT(node != nullptr);
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
