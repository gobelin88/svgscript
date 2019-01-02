#ifndef SVGVIEW_H
#define SVGVIEW_H

#include <QGraphicsView>

class QGraphicsSvgItem;
class QSvgRenderer;
class QWheelEvent;
class QPaintEvent;

class SvgView : public QGraphicsView
{
    Q_OBJECT

public:
    enum RendererType { Native, OpenGL, Image };

    explicit SvgView(QWidget *parent = nullptr);

    bool load(const QString &fileName);
    void setRenderer(RendererType type = Native);
    void drawBackground(QPainter *p, const QRectF &rect) override;

    QSize svgSize() const;
    QSvgRenderer *renderer() const;

    qreal zoomFactor() const;

public slots:
    void setHighQualityAntialiasing(bool highQualityAntialiasing);
    void setViewBackground(bool enable);
    void setViewOutline(bool enable);
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void zoomChanged();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void zoomBy(qreal factor);

    RendererType m_renderer;

    QGraphicsSvgItem *m_svgItem;
    QGraphicsRectItem *m_backgroundItem;
    QGraphicsRectItem *m_outlineItem;

    QImage m_image;
};
#endif // SVGVIEW_H
