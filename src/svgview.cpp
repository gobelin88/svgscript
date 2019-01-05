#include "svgview.h"

#include <QSvgRenderer>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QPaintEvent>
#include <QMenu>
#include <QAction>
#include <qmath.h>

#ifndef QT_NO_OPENGL
#include <QGLWidget>
#endif

SvgView::SvgView(QWidget *parent)
    : QGraphicsView(parent)
    , m_renderer(Native)
    , m_svgItem(nullptr)
    , m_backgroundItem(nullptr)
    , m_outlineItem(nullptr)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));

    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);

    // Prepare background check-board pattern
    QPixmap tilePixmap(64, 64);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(230, 230, 230);
    tilePainter.fillRect(0, 0, 32, 32, color);
    tilePainter.fillRect(32, 32, 32, 32, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
    setHighQualityAntialiasing(false);
    setViewBackground(true);
    setViewOutline(true);
}

void SvgView::showContextMenu(const QPoint &pos)
{
   QMenu contextMenu(tr("Context menu"), this);

   QAction action1("Zoom reset", this);
   QAction action2("Zoom +", this);
   QAction action3("Zoom -", this);
   QAction action4("Enable High Quality Antialiazing", this);
   QAction action5("Enable Background", this);
   QAction action6("Enable OutLine", this);
   action4.setCheckable(true);
   action4.setChecked(highQualityAntialiasing);
   action5.setCheckable(true);
   action5.setChecked(isBackgroundVisible);
   action6.setCheckable(true);
   action6.setChecked(isOutLineVisible);

   connect(&action1, SIGNAL(triggered()), this, SLOT(resetZoom()));
   connect(&action2, SIGNAL(triggered()), this, SLOT(zoomIn()));
   connect(&action3, SIGNAL(triggered()), this, SLOT(zoomOut()));
   connect(&action4, SIGNAL(triggered(bool)), this, SLOT(setHighQualityAntialiasing(bool)));
   connect(&action5, SIGNAL(triggered(bool)), this, SLOT(setViewBackground(bool)));
   connect(&action6, SIGNAL(triggered(bool)), this, SLOT(setViewOutline(bool)));

   contextMenu.addAction(&action1);
   contextMenu.addAction(&action2);
   contextMenu.addAction(&action3);
   contextMenu.addSeparator();
   contextMenu.addAction(&action4);
   contextMenu.addAction(&action5);
   contextMenu.addAction(&action6);
   contextMenu.exec(mapToGlobal(pos));
}

void SvgView::drawBackground(QPainter *p, const QRectF &)
{
    p->save();
    p->resetTransform();
    p->drawTiledPixmap(viewport()->rect(), backgroundBrush().texture());
    p->restore();
}

QSize SvgView::svgSize() const
{
    return m_svgItem ? m_svgItem->boundingRect().size().toSize() : QSize();
}

bool SvgView::load(const QString &fileName)
{
    QGraphicsScene *s = scene();

    QScopedPointer<QGraphicsSvgItem> svgItem(new QGraphicsSvgItem(fileName));
    if (!svgItem->renderer()->isValid())
        return false;

    s->clear();
    //resetTransform();

    m_svgItem = svgItem.take();
    m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_svgItem->setCacheMode(QGraphicsItem::NoCache);
    m_svgItem->setZValue(0);

    m_backgroundItem = new QGraphicsRectItem(m_svgItem->boundingRect());
    m_backgroundItem->setBrush(Qt::white);
    m_backgroundItem->setPen(Qt::NoPen);
    m_backgroundItem->setVisible(isBackgroundVisible);
    m_backgroundItem->setZValue(-1);

    m_outlineItem = new QGraphicsRectItem(m_svgItem->boundingRect());
    QPen outline(Qt::black, 2, Qt::DashLine);
    outline.setCosmetic(true);
    m_outlineItem->setPen(outline);
    m_outlineItem->setBrush(Qt::NoBrush);
    m_outlineItem->setVisible(isOutLineVisible);
    m_outlineItem->setZValue(1);

    s->addItem(m_backgroundItem);
    s->addItem(m_svgItem);
    s->addItem(m_outlineItem);

    s->setSceneRect(m_outlineItem->boundingRect().adjusted(-10, -10, 10, 10));
    return true;
}

void SvgView::setRenderer(RendererType type)
{
    m_renderer = type;

    if (m_renderer == OpenGL) {
#ifndef QT_NO_OPENGL
        setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
#endif
    } else {
        setViewport(new QWidget);
    }
}

void SvgView::setHighQualityAntialiasing(bool highQualityAntialiasing)
{
#ifndef QT_NO_OPENGL
    this->highQualityAntialiasing=highQualityAntialiasing;
    setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);
#else
    Q_UNUSED(highQualityAntialiasing);
#endif
}

void SvgView::setViewBackground(bool enable)
{
    isBackgroundVisible=enable;
    if (!m_backgroundItem)return;
    m_backgroundItem->setVisible(enable);
}

void SvgView::setViewOutline(bool enable)
{
    isOutLineVisible=enable;
    if (!m_outlineItem)return;
    m_outlineItem->setVisible(enable);
}

qreal SvgView::zoomFactor() const
{
    return transform().m11();
}

void SvgView::zoomIn()
{
    zoomBy(2);
}

void SvgView::zoomOut()
{
    zoomBy(0.5);
}

void SvgView::resetZoom()
{
    if (!qFuzzyCompare(zoomFactor(), qreal(1))) {
        resetTransform();
        emit zoomChanged();
    }
}

void SvgView::paintEvent(QPaintEvent *event)
{
    if (m_renderer == Image) {
        if (m_image.size() != viewport()->size()) {
            m_image = QImage(viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        }

        QPainter imagePainter(&m_image);
        QGraphicsView::render(&imagePainter);
        imagePainter.end();

        QPainter p(viewport());
        p.drawImage(0, 0, m_image);

    } else {
        QGraphicsView::paintEvent(event);
    }
}

void SvgView::wheelEvent(QWheelEvent *event)
{
    zoomBy(qPow(1.2, event->delta() / 240.0));
}

void SvgView::zoomBy(qreal factor)
{
    const qreal currentZoom = zoomFactor();
    if ((factor < 1 && currentZoom < 0.1) || (factor > 1 && currentZoom > 10))
        return;
    scale(factor, factor);
    emit zoomChanged();
}

QSvgRenderer *SvgView::renderer() const
{
    if (m_svgItem)
        return m_svgItem->renderer();
    return nullptr;
}
