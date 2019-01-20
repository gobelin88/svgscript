#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QTreeWidget>
#include <iostream>

#ifndef DRAWTREE_H
#define DRAWTREE_H

class EntityNode
{
public:
    EntityNode(const QBrush & brush,
               const QPen & pen,
               const QTransform & transform,
               const QPainterPath & path,
               QString name);

    void draw(QPainter & painter);

    void operator=(const EntityNode & other);

    QRectF calcBoundingRect();

    QPainterPath getTransformedPath();

public:
    QBrush brush;
    QPen pen;
    QTransform transform;
    QPainterPath path;
    QString name;
};


class Entity
{
public:
    Entity(QString name);
    void addNode(const EntityNode & node);

    Entity * copy();
    void move(QTransform t);

    void draw(QPainter & painter);

    QRectF calcBoundingRect();

public:
    QList<EntityNode> nodes;
    QString name;
};


class DrawTree
{
public:
    DrawTree();

    bool copyEntity(QString name,QTransform t);
    Entity * findEntity(QString name);
    void addEntity(QString name);
    void addNode(QPainterPath path,QString nodename);
    void addNode(QRectF rect,QString nodename);
    void addNode(QVector<QLineF> lines,QString nodename);
    void addNode(QPolygonF polygon,QString nodename);

    void clear();
    void draw(QPainter & painter);
    void populate(QTreeWidget * w_tree);

    void setBrush(const QBrush & brush);
    void setPen(const QPen & pen);
    void setTransform(const QTransform & transform);
    QTransform & getTransform(){return transform;}

    static QPainterPath rectToPath(QRectF rect);
    static QPainterPath linesToPath(QVector<QLineF> lines);
    static QPainterPath polygonToPath(QPolygonF polygon);

    QStringList brushStyles();
    QStringList penStyles();

private:
    QList<Entity> entities;

    QBrush brush;
    QPen pen;
    QTransform transform;
};

#endif // DRAWTREE_H
