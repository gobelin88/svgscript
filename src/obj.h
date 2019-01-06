#include <QtOpenGL>
#include <QGLWidget>
#include <iostream>
#include <drawable.h>

#include <Eigen/Dense>
#include <QOpenGLFunctions_1_0>

#ifndef OBJ_H
#define OBJ_H

#define TO_DEG   57.29577951308232087679
#define TO_RAD   0.017453292519943295769

using Eigen::Vector3d;
using Eigen::Vector2d;
using Eigen::Matrix2d;
using Eigen::Matrix3d;

struct Base {Vector3d U,V,W;};
typedef QList<int> Face;

class Object:public Drawable
{
public:
    Object(QString filename, double scale, Vector3d euler_angles);
    void disp();
    Vector3d getBox();
    Vector3d getBarycenter();
    double getRadius();


    void getGnomonicAll(QImage map, int res, QPainter & painter, bool meridiens, int Nlat, int Nlon, double radius, int idpN, int idpS, double W, double dL, double dA, double marge, int mode);

    void draw(QOpenGLFunctions_1_0 *f);

    void drawProj(QPainterPath &path, QPointF c, int mode);

    bool isOpen(){return open;}

private:
    Vector2d getGnomonic(Vector3d p);
    Vector2d getGnomonicProj(Base & b,Vector3d p,Vector3d bary);
    Vector3d getGnomonicProj3D(Base & b,Vector2d p,Vector3d bary);

    //Misc
    void computeNormals();
    Base getBase(const Face & f);
    Vector2d getCoord2D(const Base &b, Vector3d p, Vector3d bary);
    Vector3d getCoord3D(Base & b,Vector2d p,Vector3d bary);
    Vector3d getBarycenter(const Face &f);
    QRectF getSpan(Face f, Vector3d bary, const Base &b);
    void getFaceGnomonic(QPainter & painter,
                         QImage map,
                         int res,
                         QPointF Where,
                         int marge,
                         int id ,
                         QColor color,
                         bool meridiens,
                         QColor color_meridiens,
                         int Nlat,
                         int Nlon,
                         double radius, int idpN, int idpS,
                         double W,
                         double dL,
                         double dA,int mode);


    QLineF clamp(QLineF line, QPolygonF r);

    QList<Vector3d> pts;
    QList<Face> faces;
    QList<Vector3d> normals;
    QList<QImage> tex;
    QList< QList< Vector2d > >  texCoord;
    QList<Vector3d> wire;

    Matrix3d Rx(double ang);
    Matrix3d Ry(double ang);
    Matrix3d Rz(double ang);
    Matrix3d matFromEular(double yaw,double pitch,double roll);

    bool open;
};



#endif // OBJ_H
