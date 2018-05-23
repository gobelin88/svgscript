#include "displayer.h"
#include "drawable.h"

Displayer::Displayer(QWidget *parent, double scale): QOpenGLWidget(parent)
{
    xRot = 0;
    yRot = 0;
    zRot = 0;
    this->scale=scale;
    this->zoom=1;
}
Displayer::~Displayer()
{
    clear();
}

void Displayer::clear()
{
    for(int i=0;i<obj_list.size();i++)
    {
        delete obj_list[i];
    }
    obj_list.clear();
}

void Displayer::initializeGL()
{
    QOpenGLFunctions_1_0 *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_1_0>();
    f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    f->glEnable(GL_TEXTURE_2D);
    f->glEnable(GL_DEPTH_TEST);
    //f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_LIGHTING);
    f->glEnable(GL_LIGHT0);
    //static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    //f->glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}

void Displayer::resizeGL(int width, int height)
{
    QOpenGLFunctions_1_0 *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_1_0>();

    int side = qMin(width, height);
    f->glViewport((width - side) / 2, (height - side) / 2, side, side);
    f->glMatrixMode(GL_PROJECTION);
    f->glLoadIdentity();
    f->glOrtho(-scale, +scale, -scale, +scale, -scale, scale);
    f->glMatrixMode(GL_MODELVIEW);
}

void Displayer::paintGL()
{
    QOpenGLFunctions_1_0 *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_1_0>();

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glLoadIdentity();
    f->glTranslatef(0.0, 0.0, -10.0);
    f->glRotatef(xRot / 16.0, 1.0, 0.0, 0.0);
    f->glRotatef(yRot / 16.0, 0.0, 1.0, 0.0);
    f->glRotatef(zRot / 16.0, 0.0, 0.0, 1.0);
    f->glScaled(zoom,zoom,zoom);

    for(int i=0;i<obj_list.size();i++)
    {
        obj_list[i]->draw(f);
    }
}

void Displayer::wheelEvent(QWheelEvent *event)
{
    zoom+=event->delta()*0.002;

    std::cout<<event->delta()<<" "<<zoom<<std::endl;
    this->update();
}

void Displayer::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void Displayer::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        setXRotation(xRot + 8 * dy);
        setYRotation(yRot + 8 * dx);
    } else if (event->buttons() & Qt::RightButton) {
        setXRotation(xRot + 8 * dy);
        setZRotation(zRot + 8 * dx);
    }
    lastPos = event->pos();
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void Displayer::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != xRot) {
        xRot = angle;
        this->update();
    }
}

void Displayer::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != yRot) {
        yRot = angle;
        this->update();
    }
}

void Displayer::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != zRot) {
        zRot = angle;
        this->update();
    }
}
