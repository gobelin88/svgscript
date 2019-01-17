#include "drawtree.h"

EntityNode::EntityNode(const QBrush & brush,
                       const QPen & pen,
                       const QTransform & transform,
                       const QPainterPath & path,
                       QString name)
{
    this->brush=brush;
    this->pen=pen;
    this->transform=transform;
    this->path=path;
    this->name=name;
}
void EntityNode::draw(QPainter & painter)
{
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawPath(transform.map(path));
}

void EntityNode::operator=(const EntityNode & other)
{
    this->brush     =other.brush     ;
    this->pen       =other.pen       ;
    this->transform =other.transform ;
    this->path      =other.path      ;
    this->name      =other.name      ;
}

QRectF EntityNode::calcBoundingRect()
{
    return path.boundingRect();
}

QPainterPath EntityNode::getTransformedPath()
{
    return transform.map(path);
}

Entity::Entity(QString name)
{
    this->name=name;
}
void Entity::addNode(const EntityNode & node)
{
    nodes.append(node);
}

Entity * Entity::copy()
{
    Entity * e_copy=new Entity(this->name+QString("_copy"));
    for(int i=0;i<nodes.size();i++)
    {
        e_copy->nodes.append(this->nodes[i]);
    }
    return e_copy;
}
void Entity::move(QTransform t)
{
    for(int i=0;i<nodes.size();i++)
    {
        nodes[i].transform*=t;
    }
}

void Entity::draw(QPainter & painter)
{
    for(int i=0;i<nodes.size();i++)
    {
        nodes[i].draw(painter);
    }
}

QRectF Entity::calcBoundingRect()
{
    QPainterPath all;
    for(int i=0;i<nodes.size();i++)
    {
        all.addPath(this->nodes[i].path);
    }
    return all.boundingRect();
}

DrawTree::DrawTree()
{
    addEntity("Root");
}

void DrawTree::addEntity(QString name)
{
    this->entities.append(Entity(name));
}

Entity * DrawTree::findEntity(QString name)
{
    Entity * entity_ptr=nullptr;
    for(int i=0;i<entities.size();i++)
    {
        if(entities[i].name==name)
        {
            entity_ptr=&entities[i];
        }
    }
    return entity_ptr;
}

bool DrawTree::copyEntity(QString name,QTransform t)
{
    Entity * entity_ptr=findEntity(name);

    if(entity_ptr)
    {
        Entity * entity_copy=entity_ptr->copy();
        entity_copy->move(t);
        this->entities.append(*entity_copy);
        return true;
    }
    else
    {
        return false;
    }
}

void DrawTree::addNode(QPainterPath path,QString nodename)
{
    this->entities.back().addNode(EntityNode(brush,pen,transform,path,nodename));
}

void DrawTree::addNode(QRectF rect,QString nodename)
{
    this->entities.back().addNode(EntityNode(brush,pen,transform,rectToPath(rect),nodename));
}

void DrawTree::addNode(QVector<QLineF> lines,QString nodename)
{
    this->entities.front().addNode(EntityNode(brush,pen,transform,linesToPath(lines),nodename));
}
void DrawTree::addNode(QPolygonF polygon,QString nodename)
{
    this->entities.front().addNode(EntityNode(brush,pen,transform,polygonToPath(polygon),nodename));
}

void DrawTree::setBrush(const QBrush & brush){this->brush=brush;}
void DrawTree::setPen(const QPen & pen){this->pen=pen;}
void DrawTree::setTransform(const QTransform & transform){this->transform=transform;}

QPainterPath DrawTree::rectToPath(QRectF rect)
{
    QPainterPath path;
    path.moveTo( QPointF(rect.x(),rect.y()) );
    path.lineTo( QPointF(rect.x()+rect.width(),rect.y()) );
    path.lineTo( QPointF(rect.x()+rect.width(),rect.y()+rect.height()) );
    path.lineTo( QPointF(rect.x(),rect.y()+rect.height()) );
    path.lineTo( QPointF(rect.x(),rect.y()) );

    return path;
}

QPainterPath DrawTree::linesToPath(QVector<QLineF> lines)
{
    QPainterPath path;

    for(int i=0;i<lines.size();i++)
    {
        path.moveTo(lines[i].p1());
        path.lineTo(lines[i].p2());
    }

    return path;
}

QPainterPath DrawTree::polygonToPath(QPolygonF polygon)
{
    QPainterPath path;

    for(int i=0;i<=polygon.size();i++)
    {
        if(i==0){path.moveTo(polygon.at(0));}
        else if(i==polygon.size()){path.lineTo(polygon.at(0));}
        else{path.lineTo(polygon.at(i));}
    }

    return path;
}

void DrawTree::draw(QPainter & painter)
{
    for(int i=0;i<entities.size();i++)
    {
        entities[i].draw(painter);
    }
}

QStringList DrawTree::penStyles()
{
    QStringList styles_list;
    styles_list.append("NoPen");
    styles_list.append("SolidLine");
    styles_list.append("DashLine");
    styles_list.append("DotLine");
    styles_list.append("DashDotLine");
    styles_list.append("DashDotDotLine");
    styles_list.append("CustomDashLine");
    return styles_list;
}

QStringList DrawTree::brushStyles()
{
    QStringList styles_list;
    styles_list.append("NoBrush");
    styles_list.append("SolidPattern");
    styles_list.append("Dense1Pattern");
    styles_list.append("Dense2Pattern");
    styles_list.append("Dense3Pattern");
    styles_list.append("Dense4Pattern");
    styles_list.append("Dense5Pattern");
    styles_list.append("Dense6Pattern");
    styles_list.append("Dense7Pattern");
    styles_list.append("HorPattern");
    styles_list.append("VerPattern");
    styles_list.append("CrossPattern");
    styles_list.append("BDiagPattern");
    styles_list.append("FDiagPattern");
    styles_list.append("DiagCrossPattern");
    styles_list.append("LinearGradientPattern");
    styles_list.append("RadialGradientPattern");
    styles_list.append("ConicalGradientPattern");
    styles_list.append("TexturePattern");
    return styles_list;
}

void DrawTree::populate(QTreeWidget * w_tree)
{
    w_tree->clear();
    w_tree->setColumnCount(1);

    QList<QTreeWidgetItem *> items;
    for(int i=0;i<entities.size();i++)
    {
        QTreeWidgetItem * item_entity=new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Entity : %1").arg(entities[i].name)));

        for(int j=0;j<entities[i].nodes.size();j++)
        {
            QTreeWidgetItem * item_node=new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Node %1: %2").arg(j).arg(entities[i].nodes[j].name)));

            QString pstyle=penStyles()[entities[i].nodes[j].pen.style()];
            QString bstyle=brushStyles()[entities[i].nodes[j].brush.style()];
            QColor pcolor=entities[i].nodes[j].pen.color();
            QColor bcolor=entities[i].nodes[j].brush.color();
            float pwidth=entities[i].nodes[j].pen.widthF();

            item_node->addChild(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("pen: rvb:(%1 %2 %3) width:%4 style:%5").arg(pcolor.red()).arg(pcolor.green()).arg(pcolor.blue()).arg(pwidth).arg(pstyle))));
            item_node->addChild(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("brush: rvb:(%1 %2 %3) style:%4").arg(bcolor.red()).arg(bcolor.green()).arg(bcolor.blue()).arg(bstyle))));
            item_node->addChild(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("path lenght:%1").arg(entities[i].nodes[j].path.length()))));

            item_entity->addChild(item_node);
        }

        items.append(item_entity);
    }
    w_tree->insertTopLevelItems(0, items);
}

void DrawTree::clear()
{
    entities.clear();
}
