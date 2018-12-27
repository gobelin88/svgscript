#include "obj.h"

Object::Object(QString filename, double scale, Vector3d euler_angles)
{
    wire.clear();
    Matrix3d rmat=matFromEular(euler_angles[0],euler_angles[1],euler_angles[2]);

    QFile file(filename);

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while(!file.atEnd())
        {
            QString line_str=file.readLine();
            QStringList line=line_str.split(QRegExp("[ |\n]"),QString::SkipEmptyParts);
            if(line.size()>0)
            {
                if(line[0]=="v" && line.size()==4)
                {
                    pts.append(scale*rmat*Vector3d(line[1].toDouble(),line[2].toDouble(),line[3].toDouble()));
                }
                else if(line[0]=="f")
                {
                    Face new_face;
                    QList< Vector2d > new_face_tex_coord;
                    for(int i=1;i<line.size();i++)
                    {
                        new_face.append( line[i].toInt()-1 );
                        new_face_tex_coord.append( Vector2d (0,0) );
                    }
                    faces.append(new_face);
                    texCoord.append(new_face_tex_coord);
                }
            }
        }

        computeNormals();

        file.close();
        open=true;
    }
    else
    {
        open=false;
        std::cout<<"Impossible d'ouvrir :"<<filename.toLocal8Bit().data()<<std::endl;
    }
}

void Object::disp()
{
    std::cout<<"points :"<<std::endl;
    for(int i=0;i<pts.size();i++)
    {
        std::cout<<pts[i].x()<<" "<<pts[i].y()<<" "<<pts[i].z()<<" "<<std::endl;
    }
    std::cout<<"faces :"<<std::endl;
    for(int i=0;i<faces.size();i++)
    {
        for(int j=0;j<faces[i].size();j++)
        {
            std::cout<<faces[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
}

Vector3d Object::getBox()
{
    Vector3d minP=pts[0],maxP=pts[0];
    for(int i=1;i<pts.size();i++)
    {
        if(pts[i].x()>maxP.x())maxP.x()=pts[i].x();
        if(pts[i].x()<minP.x())minP.x()=pts[i].x();
        if(pts[i].y()>maxP.y())maxP.y()=pts[i].y();
        if(pts[i].y()<minP.y())minP.y()=pts[i].y();
        if(pts[i].z()>maxP.z())maxP.z()=pts[i].z();
        if(pts[i].z()<minP.z())minP.z()=pts[i].z();
    }
    return maxP-minP;
}

Vector3d Object::getBarycenter()
{
    Vector3d bary(0,0,0);
    for(int i=0;i<pts.size();i++){bary+=pts[i];}
    return bary/pts.size();
}

double Object::getRadius()
{
    Vector3d bary=getBarycenter();
    double radius=0.0;
    for(int i=0;i<pts.size();i++){radius+=(pts[i]-bary).norm();}
    return radius/pts.size();
}

Vector2d Object::getGnomonic(Vector3d p)
{
    //Project on the sphere
    Vector3d pn=p.normalized();
    Vector2d m( atan2(pn.y(),pn.x())/(2*M_PI)+0.5, acos(pn.z())/M_PI );
    //Get Spherical Coord
    return m;
}

Vector2d Object::getGnomonicProj(Base & b,Vector3d p,Vector3d bary)
{
    return Vector2d( bary.cross(b.V).dot(p)  , b.U.cross(bary).dot(p)  )/ b.W.dot(p);
}

Vector3d Object::getGnomonicProj3D(Base & b,Vector2d p,Vector3d bary)
{
    return p.x()*b.U+p.y()*b.V+bary;
}

void Object::computeNormals()
{
    for(int i=0;i<faces.size();i++)
    {
        Vector3d N=getBase(faces[i]).W;
        if(N.dot(getBarycenter(faces[i]))>0)
        {
            normals.push_back(N);
        }
        else
        {
            normals.push_back(-N);
        }
    }
}

Base Object::getBase(const Face & f)
{
    Vector3d pa=pts[f[0]],pb=pts[f[1]],pc=pts[f[2]];

    Base b;
    b.U=(pb-pa).normalized();
    b.V=( (pc-pa)-(pc-pa).dot(b.U)*b.U ).normalized() ;
    b.W=b.V.cross(b.U);

    if(b.W.dot(getBarycenter(f))<0)
    {
        b.W=-b.W;
        b.V=-b.V;
    }

    return b;
}

Vector2d Object::getCoord2D(const Base & b,Vector3d p,Vector3d bary)
{
    return Vector2d( b.U.dot(p-bary), b.V.dot(p-bary) );
}

Vector3d Object::getCoord3D(Base & b,Vector2d p,Vector3d bary)
{
    return bary+b.U*p.x()+b.V*p.y();
}

Vector3d Object::getBarycenter(const Face & f)
{
    Vector3d bary(0,0,0);
    for(int i=0;i<f.size();i++)
    {
        bary+=pts[f[i]];
    }
    return bary/f.size();
}

QRectF Object::getSpan(Face f,Vector3d bary,const Base & b)
{
    double minX=+1e10,maxX=-1e10;
    double minY=+1e10,maxY=-1e10;

    for(int i=0;i<f.size();i++)
    {
        Vector2d p2D=getCoord2D(b,pts[f[i]],bary);

        if(p2D.x()<minX)minX=p2D.x();
        if(p2D.x()>maxX)maxX=p2D.x();
        if(p2D.y()<minY)minY=p2D.y();
        if(p2D.y()>maxY)maxY=p2D.y();
    }

    return QRectF(minX,minY,maxX-minX,maxY-minY);
}

Vector2d clampSegment(Vector2d p,Vector2d A,Vector2d B,Vector2d U, bool & ok)
{
    Vector2d V=B-A;
    Matrix2d M;
    M.col(0)=V;M.col(1)=-U;

    Vector2d K=M.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(p-A);
    //Vector2d K=M.inverse()*(p-A);

    if(K[0]>=0 && K[0]<=1.0 && std::isnormal(K[0])){ok=true;}else{ok=false;}

    return K[0]*V+A;
}

QPointF clampPolygon(QPointF p,QPolygonF poly,QPointF U)
{
    double dmin=1e100;
    QPointF pmin(0,0);

    for(int i=0;i<poly.size();i++)
    {
        QPointF A=poly.at(i);
        QPointF B=poly.at((i+1)%poly.size());

        bool ok=false;
        Vector2d pv(p.x(),p.y());
        Vector2d pp=clampSegment(pv,
                                 Vector2d(A.x(),A.y()),
                                 Vector2d(B.x(),B.y()),
                                 Vector2d(U.x(),U.y()),ok);

        double d=(pv-pp).norm();
        if(ok)
        {
            if(d<dmin)
            {
                dmin=d;
                pmin=QPointF(pp.x(),pp.y());
            }
        }
    }
    return pmin;
}

QPointF clampP(QPointF p,QRectF r,QPointF U)
{
    if( !r.contains(p) )
    {
        if(p.x()>(r.x()+r.width ()) ) { double k=(r.x()+r.width ()-p.x() )/U.x();p = p+k*U ; }
        if(p.y()>(r.y()+r.height()) ) { double k=(r.y()+r.height()-p.y() )/U.y();p = p+k*U ; }
        if(p.x()<( r.x() ) )          { double k=(r.x()-p.x())/U.x()            ;p = p+k*U ; }
        if(p.y()<( r.y() ) )          { double k=(r.y()-p.y())/U.y()            ;p = p+k*U ; }
    }
    return p;
}

QLineF Object::clamp(QLineF line,QPolygonF poly)
{
    QPointF U(line.dx(),line.dy());

    if(!poly.containsPoint(line.p1(),Qt::WindingFill)){line.setP1(clampPolygon(line.p1(),poly,U));}
    else if(!poly.containsPoint(line.p2(),Qt::WindingFill)){line.setP2(clampPolygon(line.p2(),poly,U));}

    return line;
}

void Object::getFaceGnomonic(QPainter & painter,
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
                                 double radius,
                                 int idpN,int idpS,
                                 double W,
                                 double dL,
                                 double dA)
{
    Face f=faces[id];
    Base b=getBase(f);
    Vector3d bary=getBarycenter(f);
    QRectF span=getSpan(f,bary,b);
    Vector2d center(Where.x()-span.x(),Where.y()-span.y());

    //---------------------------------------------------------------
    QPen pencil(color);
    pencil.setWidthF(0.5);
    painter.setPen(pencil);

    QPolygonF poly;
    QPolygonF polyg;
    QList<QPolygonF> rect_list;

    for(int i=0;i<f.size();i++)
    {
        Vector2d pA=getCoord2D(b,pts[f[i]],bary);
        Vector2d pt(pA.x()+center.x(), pA.y()+center.y() );
        poly.append(QPointF(pt.x(),pt.y()));
        polyg.append(QPointF(pA.x(),pA.y()));

        texCoord[id][i]=Vector2d((pA.x()-span.x())/span.width(),(pA.y()-span.y())/span.height());

        Vector2d u=Vector2d(pt.x()-center.x(),pt.y()-center.y()).normalized();
        Vector2d v=Vector2d(-u.y(),u.x());

        QPolygonF poly_r;
        Vector2d pr;
        pr=-u*dA+pt-v*W/2; poly_r.append( QPointF(pr.x(),pr.y()) );
        pr=-u*dA+pt-u*dL-v*W/2; poly_r.append( QPointF(pr.x(),pr.y()) );
        pr=-u*dA+pt-u*dL+v*W/2; poly_r.append( QPointF(pr.x(),pr.y()) );
        pr=-u*dA+pt+v*W/2; poly_r.append( QPointF(pr.x(),pr.y()) );

        rect_list.append(poly_r);
    }

    //------------------------------------------------------------------------
    QImage img(res,res,QImage::Format_RGB32);
    img.fill(Qt::white);
    for(int i=0;i<res;i++)
    {
        for(int j=0;j<res;j++)
        {
            double x= i * span.width()/res+span.x();
            double y= j * span.height()/res+span.y();

            if( polyg.containsPoint(QPointF(x,y),Qt::OddEvenFill) )//-1 ???
            {
                Vector2d p_gno=getGnomonic(getCoord3D(b,Vector2d(x,y),bary));

                if(p_gno.x()<=1 && p_gno.y()<=1)
                {
                    img.setPixel(i,j,map.pixel( round(p_gno.x()*(map.size().width ()-1)) ,
                                                round(p_gno.y()*(map.size().height()-1)) ) );
                }
                else
                {
                    std::cout<<" err="<<p_gno.x()<<" "<<p_gno.y()<<std::endl;
                }
            }
        }
    }
    tex.append(img);

    //------------------------------------------------------------------------

    QRectF rectImage(Where-QPointF(marge,marge),span.size()+QSizeF(marge*2,marge*2));
    painter.drawImage(QRectF(round(rectImage.x()),
                             round(rectImage.y()),rectImage.width(),rectImage.height()), img );

    if(idpN==id || idpS==id)
    {
        painter.drawEllipse(QPointF(center.x(),center.y()),radius,radius);
    }

    painter.drawPolygon(poly);

    for(int i=0;i<rect_list.size();i++)
    {
        painter.drawPolygon(rect_list[i]);
    }

    //DRAW MERIDIENS--------------------------------------------------------------------------------
    if(meridiens)
    {
        QPen pencil(color_meridiens);
        pencil.setWidthF(0.2);
        painter.setPen(pencil);

        //////////////////////////////////////////////////////////////////////////////////////////////
        QList<Vector3d> list_pA,list_pB;

        for(int i=0;i<Nlat;i++)
        {
            for(int j=0;j<Nlon;j++)
            {
                double m_a=(i*2.0*M_PI)/Nlat    ,m_b=(j*M_PI)/Nlon-M_PI/2.0;
                double p_a=((i+1.0)*2.0*M_PI)/Nlat , p_b=((j+1.0)*M_PI)/Nlon-M_PI/2.0 ;
                Vector3d pA( cos(m_a)*cos(m_b), sin(m_a)*cos(m_b), sin(m_b) );
                Vector3d pB( cos(p_a)*cos(m_b), sin(p_a)*cos(m_b), sin(m_b) );
                Vector3d pC( cos(m_a)*cos(p_b), sin(m_a)*cos(p_b), sin(p_b) );

                if( b.W.dot(pA)>0 && b.W.dot(pB)>0 )
                {list_pA.push_front(pA);list_pB.push_front(pB);}

                if( b.W.dot(pA)>0 && b.W.dot(pC)>0 )
                {list_pA.push_back(pA);list_pB.push_back(pC);}
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        for(int i=0;i<list_pA.size();i++)
        {
            Vector2d ppA=getGnomonicProj(b,list_pA[i],bary);
            Vector2d ppB=getGnomonicProj(b,list_pB[i],bary);

            if( polyg.containsPoint(QPointF(ppA.x(),ppA.y()),Qt::WindingFill) ||
                polyg.containsPoint(QPointF(ppB.x(),ppB.y()),Qt::WindingFill) )
            {
                QLineF line= clamp( QLineF(ppA.x(),ppA.y(),ppB.x(),ppB.y()),polyg);
                //QLineF line= QLineF(ppA.x(),ppA.y(),ppB.x(),ppB.y());

                wire.append(getGnomonicProj3D(b,Vector2d(line.p1().x(),line.p1().y()),bary));
                wire.append(getGnomonicProj3D(b,Vector2d(line.p2().x(),line.p2().y()),bary));

                QLineF lined(center.x()+line.p1().x(),
                             center.y()+line.p1().y(),
                             center.x()+line.p2().x(),
                             center.y()+line.p2().y());

                painter.drawLine(lined);
            }
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
}

void Object::drawProj(QPainterPath & path,QPointF c,int mode)
{
    for(int i=0;i<faces.size();i++)
    {
        for(int j=0;j<faces[i].size();j++)
        {
            QPointF p;
            if(mode==0)p=QPointF(pts[faces[i][j]][0],pts[faces[i][j]][1]);
            else if(mode==1)p=QPointF(pts[faces[i][j]][1],pts[faces[i][j]][2]);
            else if(mode==2)p=QPointF(pts[faces[i][j]][0],pts[faces[i][j]][2]);

            if(j==0)
            {
                path.moveTo(c+p);
            }
            else
            {
                path.lineTo(c+p);
            }
        }
    }
}

void Object::draw(QOpenGLFunctions_1_0 * f)
{
    for(int i=0;i<faces.size();i++)
    {
        QOpenGLTexture texture(tex[i]);
        texture.bind();

        f->glColor3d(1,1,1);
        f->glNormal3d(normals[i][0],normals[i][1],normals[i][2]);
        f->glBegin(GL_POLYGON);

        for(int j=0;j<faces[i].size();j++)
        {
            f->glTexCoord2d( texCoord[i][j].x(),texCoord[i][j].y() );
            f->glVertex3d(pts[faces[i][j]][0],
                    pts[faces[i][j]][1],
                    pts[faces[i][j]][2]);
        }

        f->glEnd();

        f->glLineWidth(2);
        f->glDisable(GL_TEXTURE_2D);
        f->glDisable(GL_LIGHTING);
        f->glColor3d(1,0,0);

        f->glBegin(GL_LINE_LOOP);
        for(int j=0;j<faces[i].size();j++)
        {
            f->glTexCoord2d( texCoord[i][j].x(),texCoord[i][j].y() );
            f->glVertex3d(pts[faces[i][j]][0],
                    pts[faces[i][j]][1],
                    pts[faces[i][j]][2]);
        }
        f->glEnd();
        f->glEnable(GL_TEXTURE_2D);
        f->glEnable(GL_LIGHTING);
    }

    f->glDisable(GL_TEXTURE_2D);
    f->glDisable(GL_LIGHTING);
    f->glColor3d(0,0,1);
    f->glBegin(GL_LINES);
    for(int i=0;i<wire.size();i+=2)
    {
        f->glVertex3d(wire[i  ].x(),wire[i  ].y(),wire[i  ].z());
        f->glVertex3d(wire[i+1].x(),wire[i+1].y(),wire[i+1].z());
    }
    f->glEnd();
    f->glEnable(GL_TEXTURE_2D);
    f->glEnable(GL_LIGHTING);

}

void Object::getGnomonicAll(QImage map, int res, QPainter & painter, bool meridiens, int Nlat, int Nlon, double radius,int idpN,int idpS,double W,
                            double dL,
                            double dA,
                            double marge)
{
    int pack=(int)sqrt(faces.size())+1;

    QPointF where(marge,marge);

    double maxh=0.0;
    for(int i=0;i<faces.size();i++)
    {
        QRectF span=getSpan(faces[i],getBarycenter(faces[i]),getBase(faces[i]));
        //QImage img=getFaceGnomonic(map,Vector2d(res,res),i,span);

        getFaceGnomonic(painter,
                            map,
                            res,
                            where,
                            marge,
                            i,
                            QColor(255,0,0),
                            meridiens,
                            QColor(0,128,255),
                            Nlat,Nlon,radius,idpN,idpS,W,dL,dA);

        if((i+1)%pack==0)
        {
            where.setY(where.y()+maxh+marge*2+2);
            where.setX(marge);
            maxh=0.0;
        }
        else
        {
            where.setX(where.x()+span.width()+marge*2+2);
            if(span.height()>maxh){maxh=span.height();}
        }



        //img.save(QString("f%1.png").arg(i));
    }
}

Matrix3d Object::Rx(double ang)
{
    double c=cos(ang*TO_RAD);
    double s=sin(ang*TO_RAD);

    Matrix3d M;

    M << 1,0,0,
            0,c,-s,
            0,s,c;

    return M;
}

Matrix3d Object::Ry(double ang)
{
    double c=cos(ang*TO_RAD);
    double s=sin(ang*TO_RAD);

    Matrix3d M;

    M<<c,0,s,
            0,1,0,
            -s,0,c;

    return M;
}

Matrix3d Object::Rz(double ang)
{
    double c=cos(ang*TO_RAD);
    double s=sin(ang*TO_RAD);

    Matrix3d M;

    M<<c,-s,0,
            s,c,0,
            0,0,1;

    return M;
}

Matrix3d Object::matFromEular(double yaw,double pitch,double roll)
{
    return Ry(pitch)*Rz(roll)*Rx(yaw);
}
