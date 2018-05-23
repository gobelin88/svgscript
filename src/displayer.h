#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_1_0>

#include <QGLWidget>
#include <obj.h>
#include <QList>

#ifndef DISPLAYER_H
#define DISPLAYER_H

class Displayer:public QOpenGLWidget
{
public:
    Displayer(QWidget *parent,double scale);
    ~Displayer();

    void add(Object * pobj){obj_list.append(pobj);}
    void clear();

protected:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    QList< Object* > obj_list;

    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);

    int xRot,yRot,zRot;

    QPoint lastPos;
    double scale,zoom;
};

#endif // DISPLAYER_H
