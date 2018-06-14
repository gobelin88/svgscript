﻿#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "displayer.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    version=QString("v3.3");

    pe=NULL;

    ui->setupUi(this);

    te_script=new QTextEdit();
    te_console=new QTextEdit();

    highlighter=new Highlighter(te_script->document());

    scroll=new QScrollArea();
    w_svg=new QSvgWidget();
    pb_load=new QPushButton("Load",this);
    pb_save=new QPushButton("Save",this);
    pb_run=new QPushButton("Run (F1)",this);
    pb_save->setShortcut(tr("Ctrl+Shift+S"));
    pb_load->setShortcut(tr("Ctrl+O"));
    pb_run->setShortcut(tr("F1"));
    a_direct_save=new QAction(this);
    a_direct_save->setShortcut(tr("Ctrl+S"));
    this->addAction(a_direct_save);
    scroll->setWidget(w_svg);

    te_script->setMinimumWidth(400);
    te_script->setMaximumWidth(512);
    te_console->setMaximumSize(512,200);

    l_pb=new QHBoxLayout();
    l_pb->addWidget(pb_load);
    l_pb->addWidget(pb_save);
    l_pb->addWidget(pb_run );

    slider_layout=new QVBoxLayout();

    ui->gridLayout->addWidget(te_script,0,0);
    ui->gridLayout->addWidget(te_console,1,0);
    ui->gridLayout->addLayout(slider_layout,2,0);
    ui->gridLayout->addLayout(l_pb,3,0);
    ui->gridLayout->addWidget(scroll,0,1,4,1);

    connect(pb_load,SIGNAL(clicked()),this,SLOT(slot_load()));
    connect(pb_save,SIGNAL(clicked()),this,SLOT(slot_save()));
    connect(pb_run,SIGNAL(clicked()),this,SLOT(slot_run()));
    connect(a_direct_save,SIGNAL(triggered()),this,SLOT(slot_direct_save()));
    connect(te_script,SIGNAL(textChanged()),this,SLOT(slot_modified()));

    loadStyle(":/qss_script/rc/style.txt");


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadStyle(QString filename)
{
    QFile file(filename);
    if(file.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        this->setStyleSheet(QString(file.readAll()));
        file.close();
    }
}

void MainWindow::slot_direct_load(QString filename)
{
    if(!filename.isEmpty())
    {
        current_file_path=filename;
        te_script->clear();

        QFile file(filename);
        if(file.open(QIODevice::Text | QIODevice::ReadOnly))
        {
            te_script->setText(QString(file.readAll()));
            file.close();

            this->setWindowTitle(QString("SvgScript %1 - ").arg(version)+filename);
        }
        else
        {
            QMessageBox::critical(this,"Error","Unable to load script file");
        }
    }
}

void MainWindow::slot_load()
{
    QString filename=QFileDialog::getOpenFileName(this,"Open script",tr("./scripts/"),"*.script");
    slot_direct_load(filename);
}

void MainWindow::slot_save()
{
    QString filename=QFileDialog::getSaveFileName(this,"Save script","","*.script");
    if(filename.isEmpty())return;
    current_file_path=filename;
    slot_direct_save();
}

void MainWindow::slot_direct_save()
{
    if(current_file_path.isEmpty())return;

    QFile file(current_file_path);
    if(file.open(QIODevice::Text | QIODevice::WriteOnly))
    {
        file.write(te_script->toPlainText().toUtf8());
        file.close();

        this->setWindowTitle(QString("SvgScript %1 - ").arg(version)+current_file_path);
    }
    else
    {
        QMessageBox::critical(this,"Error","Unable to save script file");
    }
}

void MainWindow::removeSlider(QString varname)
{
    for(int i=0;i<sliderlist.size();i++)
    {
        if(sliderlist[i]->name()==varname)
        {
            slider_layout->removeWidget(sliderlist[i]);
            sliderlist.removeAt(i);
        }
    }
}

void MainWindow::slot_modified()
{
    this->setWindowTitle(QString("SvgScript %1 - ").arg(version)+current_file_path +tr("*"));
}

QString MainWindow::comment(QString content)
{
    content.remove(QRegExp("//[^\n]*"));

    QRegularExpression commentStartExpression ("/\\*");
    QRegularExpression commentEndExpression ("\\*/");

    int startIndex = 0;
    //if (previousBlockState() != 1)
    startIndex = content.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(content, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            //setCurrentBlockState(1);
            commentLength = content.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                    + match.capturedLength();
        }
        content.remove(startIndex, commentLength);
        startIndex = content.indexOf(commentStartExpression, startIndex + commentLength);
    }

    return content;
}

void MainWindow::slot_run()
{
    QScriptEngine engine;
    engine.globalObject().setProperty("$pi", M_PI);
    pe=&engine;

    QStringList content=comment(te_script->toPlainText()).split(";");

    Err err=process(content);

    if(err.line)
    {
        te_console->append(QString("Erreur Ligne %1 : %2").arg(err.line+1).arg(err.what));
    }

    w_svg->load(current_svg_path);
}




double MainWindow::exp(QString expression)
{
    return pe->evaluate(expression).toNumber();
}

double getEllipsePerimetre(double ra,double rb)
{
    double p=0.0;
    double N=100000;

    for(int i=0;i<N;i++)
    {
        double x1=ra*cos(i/N*2*M_PI),y1=rb*sin(i/N*2*M_PI);
        double x2=ra*cos((i+1)/N*2*M_PI),y2=rb*sin((i+1)/N*2*M_PI);
        p+=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
    }

    return p;
}


double evalEllipticStep(double ra,double rb,double theta,double dt)
{
    double p=0.0;
    double N=5000.0;

    double step=dt/N;

    double x1=ra*cos(theta),y1=rb*sin(theta);
    for(unsigned int k=1;k<N;k++)
    {
        double t=theta+k*step;
        double x2=ra*cos(t),y2=rb*sin(t);
        p+=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
        x1=x2;y1=y2;
    }

    return p;
}

double evalEllipticStepB(double ra,double rb,double theta,double dt)
{
    double x1=ra*cos(theta),y1=rb*sin(theta);
    double x2=ra*cos(theta+dt),y2=rb*sin(theta+dt);
    return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

double getEllipticStep(double ra,double rb,double theta,double dL)
{
    double step=0.0;
    double L=0.0;
    do
    {
        double x1=ra*cos(theta),y1=rb*sin(theta);
        step+=0.1*(dL-L)/sqrt(x1*x1+y1*y1);
        L=evalEllipticStepB(ra,rb,theta,step);
    }
    while ( std::abs(L-dL)>1e-6 );

    return step;
}

double getEllipticStepL(double ra,double rb,double dL,int N)
{
    double Lstep=dL*2;

    double theta=0,dt=0;
    do
    {
        theta=0.0;
        for(int i=0;i<N;i++)
        {
            theta+=getEllipticStep(ra,rb,theta,Lstep);
        }
        dt=2*M_PI-theta;

        if(dt>0){Lstep+=0.5*evalEllipticStep(ra,rb,theta,dt)/N;}
        else{Lstep-=0.5*evalEllipticStep(ra,rb,0,std::abs(dt))/N;}
    }
    while( std::abs(dt)>1e-6 );

    return Lstep;
}

void MainWindow::draw_ellipseCreneaux(
        QPainterPath & pts,
        const QPointF & center,
        double ra,
        double rb,
        double E,
        double dL,
        int n,
        int mode,
        double offset)
{
    if(n>=0 && n<3)return;
    if(rb<ra/10)return;
    if(ra<rb/10)return;
    if(ra<0)return;
    if(rb<0)return;

    double Le=getEllipsePerimetre(ra,rb);
    //Calcul du nombre de creneaux dans L
    int N= (n==-1) ? Le/(2.0*dL)-2.0 : n;

    double theta=0;

    double Lstep=getEllipticStepL(ra,rb,dL,N);//Le/N;

    for(int i=0;i<N;i++)
    {
        double step=getEllipticStep(ra,rb,theta,Lstep);

        double x1=ra*cos(theta),y1=rb*sin(theta);
        double x2=ra*cos(theta+step),y2=rb*sin(theta+step);
        theta+=step;

        QLineF line(center+QPointF(x1,y1),center+QPointF(x2,y2));
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());
        if(mode==2){v=-v;}

        //Calcul du reste Epsilon
        double Eps=(L-(dL))/2.0;

        //On dessine tout cela
        if(mode==2 || mode==1)
        {
            QPointF cp=line.p1();
            cp+=u*Eps+offset*u;pts.lineTo(cp);
            cp+=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
            cp-=v*E;pts.lineTo(cp);
            cp+=u*Eps-offset*u;pts.lineTo(cp);
        }
        else if(mode==3)
        {
            QPointF cp0=line.p1();
            QPointF cp1=cp0+u*Eps+offset*u;
            QPointF cp2=cp1+v*E;
            QPointF cp3=cp2+u*dL;
            QPointF cp4=cp3-v*E;
            QPointF cp5=cp4+u*Eps-offset*u;
            QPointF cp6=cp2-(u*Eps+offset*u);
            QPointF cp7=cp3+u*Eps-offset*u;

            if(i==0)
            {
                pts.moveTo(QPointF(cp6.x(),cp6.y()-2*(cp6.y()-center.y())));
            }

            pts.lineTo(cp6);
            pts.lineTo(cp2);
            pts.lineTo(cp1);
            pts.lineTo(cp0);
            //pts.lineTo(cp0);

            pts.moveTo(cp5);
            pts.lineTo(cp4);
            pts.lineTo(cp3);
            pts.lineTo(cp7);
            //pts.lineTo(cp4);
        }
    }

    if(mode==3)
    {
        //pts.moveTo(center+QPointF((ra-E/2),0));
        //pts.arcTo(QRectF(center.x()-(ra-E/2),center.y()-(rb-E/2),2*(ra-E/2),2*(rb-E/2)),0 ,360);
        pts.moveTo(center+QPointF((ra+E*1.5),0));
        pts.arcTo(QRectF(center.x()-(ra+E*1.5),center.y()-(rb+E*1.5),2*(ra+E*1.5),2*(rb+E*1.5)),0 ,360);
    }

    this->te_console->append(QString("DEFINE Lstep=%1  Nstep=%2").arg(Lstep).arg(N));
    pe->globalObject().setProperty("Lstep",Lstep);
    pe->globalObject().setProperty("Nstep",N);
}

void MainWindow::draw_lineCreneaux(QPainterPath & pts,const QLineF & line,double E,double dL,int n,int mode,double offset)
{
    if(mode==1 || mode==2)
    {
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());
        if(mode==2){v=-v;}

        //Calcul du nombre de creneaux dans L
        int N= (n==-1) ? L/(2.0*dL)-2.0 : n;

        //Calcul du reste Epsilon
        double Eps=(L-(N*2.0*dL+dL))/2.0;

        //On dessine tout cela
        QPointF cp=line.p1();
        cp+=u*Eps+offset*u;pts.lineTo(cp);

        for(int i=0;i<N;i++)
        {
            cp+=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
            cp-=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
        }
        cp+=v*E;pts.lineTo(cp);
        cp+=u*dL;pts.lineTo(cp);
        cp-=v*E;pts.lineTo(cp);
        cp+=u*Eps-offset*u;pts.lineTo(cp);
    }
    else if(mode==0)
    {
        pts.lineTo(line.p2());
    }
    else if(mode==-1)
    {
        pts.moveTo(line.p2());
    }
    else if(mode==3)
    {
        double flat=(n>=0)?dL*n:0;

        double L=line.length();
        double R=L/2-flat/2.0;
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());
        QPointF ca=line.p1()+u*L/2.0-u*flat/2.0;
        QPointF cb=line.p1()+u*L/2.0+u*flat/2.0;

        QPointF cp;
        int res=20;
        for(int i=0;i<res;i++)
        {
            double theta=M_PI/(2*res)*i;
            cp=ca-R*cos(theta)*u+R*sin(theta)*v;
            pts.lineTo(cp);
        }

        for(int i=0;i<res;i++)
        {
            double theta=M_PI/(2*res)*i+M_PI*0.5;
            cp=cb-R*cos(theta)*u+R*sin(theta)*v;
            pts.lineTo(cp);
        }
    }
    else if(mode==4 || mode==5)
    {
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());
        if(mode==5){v=-v;}

        //Calcul du nombre de crenau dans L
        int N= (n==-1) ? L/(2.0*dL)-2.0 : n;

        //Calcul du reste Epsilon
        double Eps=(L-(N*2.0*dL+dL))/2.0;

        //On dessine tout cela
        QPointF cp=line.p1();
        QPointF cbi=u*E/3;
        cp+=u*Eps+offset*u;pts.lineTo(cp+cbi);

        for(int i=0;i<N;i++)
        {
            cp+=v*E;pts.lineTo(cp-cbi);
            cp+=u*dL;pts.lineTo(cp+cbi);
            cp-=v*E;pts.lineTo(cp-cbi);
            cp+=u*dL;pts.lineTo(cp+cbi);
        }
        cp+=v*E;pts.lineTo(cp-cbi);
        cp+=u*dL;pts.lineTo(cp+cbi);
        cp-=v*E;pts.lineTo(cp-cbi);
        cp+=u*Eps-offset*u;pts.lineTo(cp);
    }
}

QVector<QLineF> getIFS(QVector<QLineF> seed, QVector<QLineF> pattern,
                       QVector<bool> done_seed, QVector<bool> done_pattern,int order)
{
    if(order==0)
    {
        return seed;
    }
    else
    {
        QVector<QLineF> seed_based;
        QVector<bool> done_based;

        for(int i=0;i<seed.size();i++)
        {
            QPointF o=seed[i].p1();
            QPointF u(seed[i].dx(),seed[i].dy());
            QPointF v(u.y(),-u.x());

            if(!done_seed[i])
            {
                for(int j=0;j<pattern.size();j++)
                {
                    seed_based.append(QLineF( o+pattern[j].p1().x()*u+pattern[j].p1().y()*v,
                                              o+pattern[j].p2().x()*u+pattern[j].p2().y()*v));
                    done_based.append(done_pattern[j]);
                }
            }
            else
            {
                seed_based.append(seed[i]);
                done_based.append(done_seed[i]);
            }
        }

        return getIFS(seed_based,pattern,done_based,done_pattern,order-1);
    }
}

QVector<QLineF> getIFS(QVector<QLineF> seed, QVector<QLineF> pattern, int order)
{
    if(order==0)
    {
        return seed;
    }
    else
    {
        QVector<QLineF> seed_based;

        for(int i=0;i<seed.size();i++)
        {
            QPointF o=seed[i].p1();
            QPointF u(seed[i].dx(),seed[i].dy());
            QPointF v(u.y(),-u.x());

            for(int j=0;j<pattern.size();j++)
            {
                seed_based.append(QLineF( o+pattern[j].p1().x()*u+pattern[j].p1().y()*v,
                                          o+pattern[j].p2().x()*u+pattern[j].p2().y()*v));
            }
        }

        return getIFS(seed_based,pattern,order-1);
    }
}

QVector<QPointF> makePolygon(QVector<QLineF> list_lines)
{
    QVector<QPointF> list_points;
    for(int i=0;i<list_lines.size();i++)
    {
        list_points.append(list_lines[i].p2());
    }
    return list_points;
}

QPainterPath getPuzzleShape(QLineF line,int mode)
{
    double L=line.length();
    QPointF u(line.dx(),line.dy());
    QPointF v(u.y(),-u.x());
    QPointF center=(line.p1()+line.p2())/2.0;
    if(mode==0)
    {
        v=-v;
    }

    QPainterPath path;

    QPointF dx=0.1*u;
    QPointF dy=-0.1*v;

    double r=0.11*L;
    double d=0.07*L;
    double s=5*r/L;

    QPointF p=line.p1(),pp;//1
    path.moveTo(p);
    pp=p;p=center-d/1.5*u/L;//2
    path.cubicTo(pp+dx,p+dy,p);
    pp=p;p=center-r*u/L+(d+r)*v/L;//3
    path.cubicTo(pp-dy,p+dy*s,p);
    pp=p;p=center+(d+2*r)*v/L;//4
    path.cubicTo(pp-dy*s,p-dx*s,p);
    pp=p;p=center+r*u/L+(d+r)*v/L;//5
    path.cubicTo(pp+dx*s,p-dy*s,p);
    pp=p;p=center+d/1.5*u/L;//6
    path.cubicTo(pp+dy*s,p-dy,p);
    pp=p;p=line.p2();//7
    path.cubicTo(pp+dy,p-dx,p);

    return path;
}

QPainterPath getRosaceElement(QLineF line,double du,double r)
{
    QPainterPath path;

    double L=line.length();
    QPointF u(line.dx(),line.dy());
    QPointF v(u.y(),-u.x());
    QPointF center=(line.p1()+line.p2())/2.0;

    QPointF A=line.p1()+u/L*du;
    QPointF B=line.p2()-u/L*du;

    double y=(L-2*du)/2.0;
    double x=sqrt(r*r-y*y);

    std::cout<<x<<" "<<y<<" "<<r<<std::endl;

    QPointF C1=center+v/L*x;
    QPointF C2=center-v/L*x;

    //path.addEllipse(C1.x()-r,C1.y()-r,2*r,2*r);
    //path.addEllipse(C2.x()-r,C2.y()-r,2*r,2*r);

    double dtheta=2*atan(y/x)*180/M_PI;

    QPointF V1=(A-C1)/r;
    QPointF V2=(B-C2)/r;

    path.moveTo(A);

    std::cout<<atan2(-V1.y(),V1.x())*180/M_PI<<std::endl;

    path.arcTo(C1.x()-r,C1.y()-r,2*r,2*r,atan2(-V1.y(),V1.x())*180/M_PI, dtheta);
    path.arcTo(C2.x()-r,C2.y()-r,2*r,2*r,atan2(-V2.y(),V2.x())*180/M_PI, dtheta);


    return path;
}

QPointF TR(QPointF pt,QRectF rect)
{
    QPointF p=pt;
    if(p.x()<rect.left()    )p.setX(rect.left());
    if(p.x()>rect.right()   )p.setX(rect.right());
    if(p.y()>rect.bottom()  )p.setY(rect.bottom());
    if(p.y()<rect.top()     )p.setY(rect.top());
    return p;
}

int nearest(QPointF p,const QVector<QLineF> & lines,bool & revert)
{
    double d=1e100;
    int id=0;
    for(int i=0;i<lines.size();i++)
    {
        QPointF pd=(lines[i].p1()-p);
        double D = sqrt(pd.x()*pd.x()+pd.y()*pd.y());
        if( D < d )
        {
            d=D;
            id=i;
            revert=false;
        }

        pd=(lines[i].p2()-p);
        D = sqrt(pd.x()*pd.x()+pd.y()*pd.y());
        if( D < d )
        {
            d=D;
            id=i;
            revert=true;
        }

    }
    return id;
}

QVector<QLineF> sortLines(QPointF p,QVector<QLineF> lines)
{
    QVector<QLineF> lines_sorted;

    while(lines.size()>0)
    {
        bool revert;
        int id=nearest(p,lines,revert);

        if(revert==false)
        {
            lines_sorted.push_back(lines[id]);
            p=lines[id].p2();
        }
        else
        {
            lines_sorted.push_back(QLineF(lines[id].p2(),lines[id].p1()));
            p=lines[id].p1();
        }

        lines.remove(id);
    }

    return lines_sorted;
}

QPainterPath roundRectPath(const QRectF &rect,double radius)
{
    double diam = 2 * radius;

    double x1, y1, x2, y2;
    rect.getCoords(&x1, &y1, &x2, &y2);

    QPainterPath path;
    path.moveTo(x2, y1 + radius);
    path.arcTo(QRectF(x2 - diam, y1, diam, diam), 0.0, +90.0);
    path.lineTo(x1 + radius, y1);
    path.arcTo(QRectF(x1, y1, diam, diam), 90.0, +90.0);
    path.lineTo(x1, y2 - radius);
    path.arcTo(QRectF(x1, y2 - diam, diam, diam), 180.0, +90.0);
    path.lineTo(x1 + radius, y2);
    path.arcTo(QRectF(x2 - diam, y2 - diam, diam, diam), 270.0, +90.0);
    path.closeSubpath();
    return path;
}

void MainWindow::calcGnomonicProjection(QPainter & painter,
                                        QString obj_filename,
                                        QString map_filename,
                                        int res,
                                        float scale,
                                        bool meridiens,
                                        int Nlat,
                                        int Nlon,
                                        Vector3d euler_angles,
                                        double radius,int idpN,int idpS,double W,double dL,double dA,double marge)
{
    Object * pobj=new Object(obj_filename,scale,euler_angles);


    Vector3d bary=pobj->getBarycenter();
    Vector3d bbox=pobj->getBox();

    te_console->append(QString("OBJ--------------------------------------"));
    te_console->append(QString("-Diametre=%1").arg(pobj->getRadius()*2));
    te_console->append(QString("-Barycentre=(%1,%2,%3)").arg(bary.x()).arg(bary.y()).arg(bary.z()));
    te_console->append(QString("-Bounding Box=(%1,%2,%3)").arg(bbox.x()).arg(bbox.y()).arg(bbox.z()));
    te_console->append(QString("-----------------------------------------"));

    QImage map(map_filename);

    displayer=new Displayer(NULL,pobj->getRadius()*1.1);
    displayer->setFixedSize(512,512);
    displayer->show();

    //pobj->disp();

    pobj->getGnomonicAll(map,res,painter,meridiens,Nlat,Nlon,radius,idpN,idpS,W,dL,dA,marge);
    std::cout<<std::endl;

    displayer->add(pobj);
}

void MainWindow::draw_gear(QPainterPath & path,double x,double y,double dtooth,int ntooth,double e,double a)
{
    if(ntooth>0)
    {
        double dt=360.0/ntooth;
        double R=ntooth*dtooth/(2*M_PI);
        double Re=R+e;

        path.arcMoveTo(x-R,y-R,2*R,2*R,dt*(a*0.25));
        for(int i=0;i<ntooth;i++)
        {
            path.arcTo(x-R,y-R,2*R,2*R,dt*(i+a*0.25),dt*(0.5-a*0.5));
            path.arcTo(x-Re,y-Re,2*Re,2*Re,dt*(i+0.5+a*0.25),dt*(0.5-a*0.5));
        }
        path.lineTo(x+R*cos(dt*(a*0.25)*M_PI/180),y-R*sin(dt*(a*0.25)*M_PI/180));

        pe->globalObject().setProperty("R", R);
        this->te_console->append(QString("DEFINE R=%1").arg(R));
    }
}

Err MainWindow::process(QStringList content)
{
    transform.reset();

    te_console->clear();
    QSvgGenerator generator;
    //EpsPaintDevice generator;
    QPainter painter;

    for(int i=0;i<content.size();i++)
    {
        QStringList args=content[i].split(QRegExp(" |\n"),QString::SkipEmptyParts);

        if(args.size()>0)
        {
            //te_console->append(QString("(%1,%2)=%3 :").arg(i).arg(args.size()).arg(args[0].toLocal8Bit().data()));

            if(args.size()==6 && args[0]==QString("SVG_LOAD"))//Ok
            {
                QRectF area(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4]));
                QSvgRenderer svg_renderer;
                svg_renderer.load(args[5]);
                svg_renderer.render(&painter,area);
            }
            else if(args.size()==6 && args[0]==QString("IMG_LOAD"))//Ok
            {
                QPointF Op(exp(args[1]),exp(args[2]));
                QPointF Sp(exp(args[3]),exp(args[4]));
                QPointF Cp=transform.map(Op+Sp/2);

                QRectF area(Cp-Sp/2,Cp+Sp/2);

                QImage img(args[5]);
                painter.drawImage(area,img);
            }
            else if(args.size()==3 && args[0]==QString("SVG_BEGIN"))//Ok
            {
                //SVG
                double scale=generator.resolution()/25.4;

                QFileInfo info(current_file_path);

                current_svg_path=QString("%1.svg").arg(info.baseName());////args[1];

                generator.setFileName(current_svg_path);
                QSize size(exp(args[1])*scale, exp(args[2])*scale);
                generator.setSize(size);
                generator.setViewBox(QRectF(0, 0, exp(args[1]), exp(args[2])));
                generator.setTitle(args[1]);
                generator.setDescription(tr("An SVG drawing created by the SVG Generator "));

                //EPS
                //                current_svg_path=args[1];
                //                generator.setFileName(current_svg_path);
                //                QSize size(exp(args[2]), exp(args[3]));
                //                generator.setSize(size);

                painter.begin(&generator);
                w_svg->setFixedSize(size);
            }
            else if(args.size()==1 && args[0]==QString("SVG_END"))//Ok
            {
                painter.end();
            }
            else if(args.size()==6 && args[0]==QString("SVG_PEN"))//Ok
            {
                QPen pen;
                pen.setColor( QColor(exp(args[1]),exp(args[2]),exp(args[3])) );
                pen.setWidthF(exp(args[4]));

                if(args[5]==QString("None")){pen.setStyle(Qt::NoPen);}
                else if (args[5]==QString("Solid")){pen.setStyle(Qt::SolidLine);}
                else if (args[5]==QString("Dash")){pen.setStyle(Qt::DashLine);}
                else if (args[5]==QString("Dot")){pen.setStyle(Qt::DotLine);}
                else {return Err(i,args[0]+QString(" : Type inconnu"));}

                painter.setPen(pen);
            }
            else if(args.size()==5 && args[0]==QString("SVG_BRUSH"))//Ok
            {
                QBrush brush;
                brush.setColor( QColor(exp(args[1]),exp(args[2]),exp(args[3])) );
                if(args[4]==QString("None")){brush.setStyle(Qt::NoBrush);}
                else if (args[4]==QString("Solid")){brush.setStyle(Qt::SolidPattern);}
                else {return Err(i,args[0]+QString(" : Type inconnu"));}

                painter.setBrush(brush);
            }
            else if(args.size()==8 && args[0]==QString("DRAW_CIRCLE_CRENEAUX"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double r=exp(args[3]);

                path.moveTo(c+QPointF(r,0));
                draw_ellipseCreneaux(path,c,r,r,exp(args[4]),exp(args[5]),exp(args[6]),exp(args[7]));

                painter.drawPath(transform.map(path));
            }
            else if(args.size()==9 && args[0]==QString("DRAW_ELLIPSE_CRENEAUX"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double ra=exp(args[3]),rb=exp(args[4]);

                path.moveTo(c+QPointF(ra,0));
                draw_ellipseCreneaux(path,c,ra,rb,exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]));

                painter.drawPath(transform.map(path));
            }
            else if(args.size()>=4 && args[0]==QString("DRAW_CIRCLE"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double r=exp(args[3]);

                if(args.size()==4){path.moveTo(c+QPointF(r,0)); path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),0 ,360);}
                else if(args.size()==5){path.moveTo(c+QPointF(r*cos(exp(args[4])*TO_RAD),-r*sin(exp(args[4])*TO_RAD)));path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),exp(args[4]) ,360);}
                else if(args.size()==6){path.moveTo(c+QPointF(r*cos(exp(args[4])*TO_RAD),-r*sin(exp(args[4])*TO_RAD)));path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),exp(args[4]) ,exp(args[5]));}

                painter.drawPath(transform.map(path));
            }
            else if(args.size()>=5 && args[0]==QString("DRAW_ELLIPSE"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double ra=exp(args[3]),rb=exp(args[4]);

                if(args.size()==5){path.moveTo(c+QPointF(ra,0));path.arcTo(QRectF(c.x()-ra,c.y()-rb,2*ra,2*rb),0 ,360);}
                else if(args.size()==6){path.moveTo(c+QPointF(ra*cos(exp(args[5])*TO_RAD),-rb*sin(exp(args[5])*TO_RAD)));path.arcTo(QRectF(c.x()-ra,c.y()-rb,2*ra,2*rb),exp(args[5]) ,360);}
                else if(args.size()==7){path.moveTo(c+QPointF(ra*cos(exp(args[5])*TO_RAD),-rb*sin(exp(args[5])*TO_RAD)));path.arcTo(QRectF(c.x()-ra,c.y()-rb,2*ra,2*rb),exp(args[5]) ,exp(args[6]));}

                painter.drawPath(transform.map(path));
            }
            else if(args.size()==7 && args[0]==QString("DRAW_LINE_ROSACE"))//Ok
            {
                painter.drawPath(transform.map(getRosaceElement(QLineF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])),exp(args[5]),exp(args[6]))));
            }
            else if(args.size()==5 && args[0]==QString("DRAW_LINE"))//Ok
            {
                QPainterPath path;
                path.moveTo( QPointF(exp(args[1]),exp(args[2])) );
                path.lineTo( QPointF(exp(args[3]),exp(args[4])) );
                painter.drawPath(transform.map(path));
            }
            else if(args.size()>=5 && args[0]==QString("DRAW_PATH"))//Ok
            {
                int n=args.size();

                QPainterPath path;
                for(int k=1;k<n;)
                {
                    if(args[k]=="M")
                    {
                        path.moveTo(QPointF(exp(args[k+1]),exp(args[k+2])));
                        k+=3;
                    }
                    else if(args[k]=="L")
                    {
                        path.lineTo(QPointF(exp(args[k+1]),exp(args[k+2])));
                        k+=3;
                    }
                    else if(args[k]=="C" && (k+5)<args.size())
                    {
                        QPointF c= QPointF(exp(args[k+1]),exp(args[k+2]) );
                        double r=exp(args[k+3]);
                        path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),exp(args[k+4]) ,exp(args[k+5]));
                        k+=6;
                    }
                    else if(args[k]=="E" && (k+6)<args.size())
                    {
                        QPointF c=QPointF(exp(args[k+1]),exp(args[k+2]) );
                        double r1=exp(args[k+3]);
                        double r2=exp(args[k+4]);
                        path.arcTo(QRectF(c.x()-r1,c.y()-r2,2*r1,2*r2),exp(args[k+5]),exp(args[k+6]));
                        k+=7;
                    }
                    else
                    {
                        return Err(k,args[0]+QString(" : mauvais arguments (%1)").arg(args[k]));
                    }
                }
                painter.drawPath(transform.map(path));
            }
            else if(args.size()>=5 && args[0]==QString("DRAW_PATH_R"))//Ok
            {
                int n=args.size();

                QPainterPath path;
                QPointF P;

                for(int k=1;k<n;)
                {
                    if(args[k]=="M")
                    {
                        P=QPointF(exp(args[k+1]),exp(args[k+2]));
                        path.moveTo(P);
                        k+=3;
                    }
                    else if(args[k]=="L")
                    {
                        P=P+QPointF(exp(args[k+1]),exp(args[k+2]));
                        path.lineTo(P);
                        k+=3;
                    }
                    else if(args[k]=="C" && (k+5)<args.size())
                    {
                        P=P+QPointF(exp(args[k+1]),exp(args[k+2]));
                        double r=exp(args[k+3]);
                        path.arcTo(QRectF(P.x()-r,P.y()-r,2*r,2*r),exp(args[k+4]),exp(args[k+5]));
                        k+=6;
                    }
                    else if(args[k]=="E" && (k+6)<args.size())
                    {
                        P=P+QPointF(exp(args[k+1]),exp(args[k+2]));
                        double r1=exp(args[k+3]);
                        double r2=exp(args[k+4]);
                        path.arcTo(QRectF(P.x()-r1,P.y()-r2,2*r1,2*r2),exp(args[k+5]),exp(args[k+6]));
                        k+=7;
                    }
                    else
                    {
                        return Err(k,args[0]+QString(" : mauvais arguments (%1)").arg(args[k]));
                    }
                }
                painter.drawPath(transform.map(path));
            }
            else if(args.size()>=6 && args[0]==QString("DRAW_POLYGON"))//Ok
            {
                QPointF c=QPointF(exp(args[1]),exp(args[2]));
                double R=exp(args[3]);
                int n=exp(args[4]);
                double dt=2*M_PI/n;
                double t=exp(args[5])*M_PI/180.0;

                QVector<QPointF> vpoints;
                for(int j=0;j<n;j+=1)
                {
                    vpoints.append(c+R*QPointF(cos(j*dt+t),sin(j*dt+t)));
                }

                QPolygonF polygon(vpoints);
                painter.drawPolygon(transform.map(polygon));
            }
            else if(args.size()==2 && args[0]==QString("DISP"))//Ok
            {
                te_console->append(QString("%1=%2").arg(args[1]).arg(exp(args[1])));
            }
            else if(args.size()==5 && args[0]==QString("SLIDE"))
            {
                MyQSlider * slider=NULL;
                for(int i=0;i<sliderlist.size();i++)
                {
                    if(sliderlist[i]->name()==args[1])
                    {
                        slider=sliderlist[i];
                    }
                }

                if(!slider)
                {
                    slider=new MyQSlider(args[1],exp(args[3]),exp(args[4]),this);
                    sliderlist.append(slider);
                    connect(slider,SIGNAL(deleted(QString)),this,SLOT(removeSlider(QString)));
                    slider_layout->addWidget(slider);
                    slider->set_value(exp(args[2]));
                    connect(slider->obj(),SIGNAL(valueChanged(int)),slider,SLOT(updateValue(int)));

                    pe->globalObject().setProperty(slider->name(),slider->get_value());
                }

                pe->globalObject().setProperty(slider->name(),slider->get_value());
            }
            else if(args.size()==3 && args[0]==QString("DEFINE"))//Ok
            {
                pe->globalObject().setProperty(args[1], exp(args[2]));
            }
            else if(args.size()==5 && args[0]==QString("DRAW_RECT"))//Ok
            {
                //painter.drawRect( QRectF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])) );
                //painter.drawLine( transform.map(QLineF(exp(args[1]),exp(args[2]),exp(args[1])+exp(args[3]),exp(args[2]) ) ));
                //painter.drawLine( transform.map(QLineF(exp(args[1])+exp(args[3]),exp(args[2]),exp(args[1])+exp(args[3]),exp(args[2])+exp(args[4]) ) ));
                //painter.drawLine( transform.map(QLineF(exp(args[1])+exp(args[3]),exp(args[2])+exp(args[4]),exp(args[1]),exp(args[2])+exp(args[4]) ) ));
                //painter.drawLine( transform.map(QLineF(exp(args[1]),exp(args[2])+exp(args[4]),exp(args[1]),exp(args[2]) ) ));

                QPainterPath path;
                path.moveTo( QPointF(exp(args[1]),exp(args[2])) );
                path.lineTo( QPointF(exp(args[1])+exp(args[3]),exp(args[2])) );
                path.lineTo( QPointF(exp(args[1])+exp(args[3]),exp(args[2])+exp(args[4])) );
                path.lineTo( QPointF(exp(args[1]),exp(args[2])+exp(args[4])) );
                path.lineTo( QPointF(exp(args[1]),exp(args[2])) );
                painter.drawPath(transform.map(path));
            }
            else if(args.size()==6 && args[0]==QString("DRAW_RECT_ROUND"))//Ok
            {
                painter.drawPath(transform.map(roundRectPath(QRectF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])),exp(args[5]))));
            }
            else if(args.size()==6 && args[0]==QString("DRAW_PUZZLE"))//Ok
            {
                double x=exp(args[1]);
                double y=exp(args[2]);

                int nx=exp(args[3]);
                int ny=exp(args[4]);
                double sz=exp(args[5]);
                double lx=nx*sz;
                double ly=ny*sz;

                for(int i=1;i<nx;i++)
                {
                    for(int j=0;j<ny;j++)
                    {
                        painter.drawPath(getPuzzleShape(QLine(x+i*sz,y+j*sz,x+i*sz,y+(j+1)*sz),rand()%2));
                    }
                }
                for(int i=1;i<ny;i++)
                {
                    for(int j=0;j<nx;j++)
                    {
                        painter.drawPath(getPuzzleShape(QLine(x+j*sz,y+i*sz,x+(j+1)*sz,y+i*sz),rand()%2));
                    }
                }

                painter.drawRect(QRectF(x,y,lx,ly));

            }
            else if(args.size()==1 && args[0]==QString("HELP"))//Ok
            {
                for(int i=0;i<NB_CMD;i++)
                {
                    te_console->append(CommandsList[i].keyword);
                }
            }
            else if(args.size()==2 && args[0]==QString("HELP"))//Ok
            {
                bool found=false;
                for(int i=0;i<NB_CMD;i++)
                {
                    if(CommandsList[i].keyword==args[1] || args[1]=="all")
                    {
                        te_console->append(QString("%1 %2\n").arg(CommandsList[i].keyword).arg(CommandsList[i].help));
                        found=true;
                    }
                }
                if(!found)
                {
                    te_console->append(QString("%1 : Not Found").arg(args[1]));
                }
            }
            else if(args.size()==7 && args[0]==QString("DRAW_FLEX"))//Ok
            {
                double W=exp(args[5]);
                double dL=exp(args[6]);
                QPointF size(exp(args[3]),exp(args[4]));
                QPointF p1(exp(args[1]),exp(args[2]));
                QPointF p2=p1+size;
                QPointF c=p1+size*0.5;
                QPointF p(0,dL/2);

                QVector<QLineF> lines;

                for(int j=0;j<size.x()/(2*W);j++)
                {
                    int odd=(j%2);
                    for(int i=0;i<size.y()/(2*(dL+dL/4));i++)
                    {
                        QPointF o,o2,Pa,Pb,Pc,Pd;

                        if(!odd)
                        {
                            QRectF rect=QRectF(p1,p2);
                            p=QPointF(0,dL/2);
                            o=QPointF(0,i*(dL+dL/4.0));
                            o2=QPointF(j*W,0);
                            Pa=TR(c+o+o2-p,rect);Pb=TR(c+o+o2+p,rect);
                            Pc=TR(c-o+o2-p,rect);Pd=TR(c-o+o2+p,rect);


                            lines.push_back(QLineF(Pa,Pb));
                            if(i>0)lines.push_back(QLineF(Pc,Pd));

                            if(j>0)
                            {
                                o2=QPointF(-j*W,0);
                                Pa=TR(c+o+o2-p,rect);Pb=TR(c+o+o2+p,rect);
                                Pc=TR(c-o+o2-p,rect);Pd=TR(c-o+o2+p,rect);
                                lines.push_back(QLineF(Pa,Pb));
                                if(i>0)lines.push_back(QLineF(Pc,Pd));
                            }

                        }
                        else
                        {
                            QPointF m(0,W);
                            QRectF rect=QRectF(p1+m,p2-m);
                            p=QPointF(0,dL);
                            o=QPointF(0,i*(dL+dL/4.0)+dL/8.0);
                            o2=QPointF(j*W,0);
                            Pa=TR(c+o+o2,rect);Pb=TR(c+o+o2+p,rect);
                            Pc=TR(c-o+o2-p,rect);Pd=TR(c-o+o2,rect);
                            lines.push_back(QLineF(Pa,Pb));
                            lines.push_back(QLineF(Pc,Pd));

                            o2=QPointF(-j*W,0);
                            Pa=TR(c+o+o2,rect);Pb=TR(c+o+o2+p,rect);
                            Pc=TR(c-o+o2-p,rect);Pd=TR(c-o+o2,rect);
                            lines.push_back(QLineF(Pa,Pb));
                            lines.push_back(QLineF(Pc,Pd));
                        }
                    }
                }

                lines=sortLines(p1,lines);
                painter.drawLines(lines);

                //                for(int i=0;i<lines.size();i++)
                //                {
                //                    painter.drawText(lines[i].p1(),QString::number(i));
                //                }
            }
            else if(args.size()==9 && args[0]==QString("DRAW_LINE_CRENEAUX"))//Ok
            {
                QPainterPath path;
                double x1=exp(args[1]);
                double y1=exp(args[2]);
                double x2=exp(args[3]);
                double y2=exp(args[4]);


                path.moveTo(QPointF(x1,y1));
                draw_lineCreneaux(path,
                                  QLineF(QPointF(x1,y1),QPointF(x2,y2)),
                                  exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]));

                painter.drawPath(transform.map(path));
            }
            else if(args.size()==15 && args[0]==QString("DRAW_RECT_CRENEAUX"))//Ok
            {
                QPainterPath path;

                double x=exp(args[1]);
                double y=exp(args[2]);
                double w=exp(args[3]);
                double h=exp(args[4]);

                QLineF lineS(x  ,y  ,x+w,y  );//Sud
                QLineF lineO(x  ,y+h,x  ,y  );//Ouest
                QLineF lineN(x+w,y+h,x  ,y+h);//Nord
                QLineF lineE(x+w,y  ,x+w,y+h);//Est

                path.moveTo(QPointF(x,y));
                draw_lineCreneaux(path,lineS,exp(args[5]),exp(args[6]),exp(args[7]),exp(args[11]));
                draw_lineCreneaux(path,lineE,exp(args[5]),exp(args[6]),exp(args[8]),exp(args[12]));
                draw_lineCreneaux(path,lineN,exp(args[5]),exp(args[6]),exp(args[9]),exp(args[13]));
                draw_lineCreneaux(path,lineO,exp(args[5]),exp(args[6]),exp(args[10]),exp(args[14]));
                painter.drawPath(transform.map(path));
            }
            else if(args.size()==19 && args[0]==QString("DRAW_RECT_CRENEAUX"))//Ok
            {
                QPainterPath path;
                double x=exp(args[1]);
                double y=exp(args[2]);
                double w=exp(args[3]);
                double h=exp(args[4]);

                QLineF lineS(x  ,y  ,x+w,y  );//Sud
                QLineF lineO(x  ,y+h,x  ,y  );//Ouest
                QLineF lineN(x+w,y+h,x  ,y+h);//Nord
                QLineF lineE(x+w,y  ,x+w,y+h);//Est

                path.moveTo(QPointF(x,y));
                draw_lineCreneaux(path,lineS,exp(args[5]),exp(args[6]),exp(args[7]),exp(args[11]),exp(args[15]));
                draw_lineCreneaux(path,lineE,exp(args[5]),exp(args[6]),exp(args[8]),exp(args[12]),exp(args[16]));
                draw_lineCreneaux(path,lineN,exp(args[5]),exp(args[6]),exp(args[9]),exp(args[13]),exp(args[17]));
                draw_lineCreneaux(path,lineO,exp(args[5]),exp(args[6]),exp(args[10]),exp(args[14]),exp(args[18]));
                painter.drawPath(transform.map(path));
            }
            else if(args.size()==2 && args[0]==QString("LABEL"))//Ok
            {
                //Just a label
            }
            else if(args.size()==3 && args[0]==QString("GOTO"))//Ok
            {
                for(int j=0;j<content.size();j++)
                {
                    QStringList argslist=content[j].split(QRegExp(" |\n"),QString::SkipEmptyParts);

                    if(argslist.size()==2)
                    {
                        if(argslist[0]==QString("LABEL") && argslist[1]==args[1])
                        {
                            pe->globalObject().setProperty(args[1], exp(args[1])+1);
                            if(exp(args[1])<exp(args[2]))
                            {
                                i=j;
                            }
                            break;
                        }
                    }
                }
            }
            else if(args.size()==7 && args[0]==QString("FRACTALE_TREE"))//Ok
            {
                QVector<bool> done_seed;
                QVector<bool> done_pattern;
                QVector<QLineF> seed;
                QVector<QLineF> pattern;

                double S=exp(args[3]);
                QPointF Po(exp(args[1]),exp(args[2]));
                QPointF Pa=Po+QPoint(0,-S);

                seed.append(QLineF(Po,Pa));
                done_seed.append(false);

                double t=M_PI/180*exp(args[5]);
                double r=exp(args[6]);
                pattern.append(QLineF(0      ,0  ,r,0));
                pattern.append(QLineF(r,0.0,r+(1-r)*cos(t), (1-r)*sin(t)));
                pattern.append(QLineF(r,0.0,r+(1-r)*cos(t),-(1-r)*sin(t)));


                done_pattern.append(true);
                done_pattern.append(false);
                done_pattern.append(false);

                QVector<QLineF> vlines=getIFS(seed,pattern,done_seed,done_pattern,exp(args[4]));
                painter.drawLines(vlines);
            }
            else if(args.size()==5 && args[0]==QString("FRACTALE_VON_KOCH"))
            {
                QVector<QLineF> seed;
                QVector<QLineF> pattern;

                double S=exp(args[3]);
                QPointF Po(exp(args[1]),exp(args[2]));
                QPointF Pa=Po+QPoint(-S/2,-sqrt(3.0)/6*S);
                QPointF Pb=Po+QPoint( S/2,-sqrt(3.0)/6*S);
                QPointF Pc=Po+QPoint( 0  ,S/sqrt(3.0));

                seed.append(QLineF(Pa,Pb));
                seed.append(QLineF(Pb,Pc));
                seed.append(QLineF(Pc,Pa));

                pattern.append(QLineF(0      ,0  ,1.0/3.0,0));
                pattern.append(QLineF(1.0/3.0,0.0,0.5    ,0.2886));
                pattern.append(QLineF(0.5    ,0.2886  ,2.0/3.0,0));
                pattern.append(QLineF(2.0/3.0,0  ,1      ,0));

                QVector<QPointF> vpoints=makePolygon(getIFS(seed,pattern,exp(args[4])));
                QPolygonF polygon(vpoints);
                painter.drawPolygon(polygon);
            }
            else if(args.size()==5 && args[0]==QString("FRACTALE_SIERPINSKI"))
            {
                QVector<QLineF> seed;
                QVector<QLineF> pattern;

                double S=exp(args[3]);
                QPointF Po(exp(args[1]),exp(args[2]));
                QPointF Pa=Po+QPoint(-S/2,-sqrt(3.0)/6*S);
                QPointF Pb=Po+QPoint( S/2,-sqrt(3.0)/6*S);
                QPointF Pc=Po+QPoint( 0  ,S/sqrt(3.0));

                seed.append(QLineF(Pa,Pb));
                seed.append(QLineF(Pb,Pc));
                seed.append(QLineF(Pc,Pa));

                pattern.append(QLineF(0.0,0.0,0.5,0.0));
                pattern.append(QLineF(0.5,0.0,0.25,-0.5*sqrt(3.0)/2));
                pattern.append(QLineF(0.5,0.0,1.0,0.0));

                QVector<QLineF> vlines=getIFS(seed,pattern,exp(args[4]));
                painter.drawLines(vlines);
            }
            else if(args.size()==5 && args[0]==QString("FRACTALE_PYTHAGORE"))
            {
                QVector<QLineF> seed;
                QVector<QLineF> pattern;

                double S=exp(args[3]);
                QPointF Po(exp(args[1]),exp(args[2]));
                QPointF Pa=Po+QPoint(-S/2,0);
                QPointF Pb=Po+QPoint( S/2,0);

                seed.append(QLineF(Pa,Pb));

                pattern.append(QLineF(0.0,0.0,0.5,0.5));
                pattern.append(QLineF(0.5,0.5,1.0,0.0));

                QVector<QLineF> vlines=getIFS(seed,pattern,exp(args[4]));
                painter.drawLines(vlines);
            }
            else if(args.size()==5 && args[0]==QString("FRACTALE_DRAGON"))
            {
                QVector<QLineF> seed;
                QVector<QLineF> pattern;

                double S=exp(args[3]);
                QPointF Po(exp(args[1]),exp(args[2]));
                QPointF Pa=Po+QPoint(-S/2,0);
                QPointF Pb=Po+QPoint( S/2,0);

                seed.append(QLineF(Pa,Pb));

                pattern.append(QLineF(0.0,0.0,0.5,0.5));
                pattern.append(QLineF(1.0,0.0,0.5,0.5));

                QVector<QLineF> vlines=getIFS(seed,pattern,exp(args[4]));
                painter.drawLines(vlines);
            }
            else if (args.size()==4 && args[0]==QString("TRANSFORM_ROTATE"))
            {
                QTransform t;
                t.translate(exp(args[1]),exp(args[2]));
                t.rotate(exp(args[3]));
                t.translate(-exp(args[1]),-exp(args[2]));
                transform*=t;
            }
            else if (args.size()==3 && args[0]==QString("TRANSFORM_TRANSLATE"))
            {
                QTransform t;
                t.translate(exp(args[1]),exp(args[2]));
                transform*=t;
            }
            else if (args.size()==3 && args[0]==QString("TRANSFORM_SCALE"))
            {
                QTransform t;
                t.scale(exp(args[1]),exp(args[2]));
                transform*=t;
            }
            else if (args.size()==3 && args[0]==QString("TEXT_FONT"))
            {
                QFont font(args[1],exp(args[2]));
                painter.setFont(font);
            }
            else if (args.size()>=4 && args[0]==QString("TEXT"))
            {
                QPainterPath path;

                if(args.size()==4)
                {
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),args[3]);
                }
                else if(args.size()==5)
                {
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),args[3].arg(exp(args[4])));
                }
                else if(args.size()==6)
                {
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),args[3].arg(exp(args[4])).arg(exp(args[5])));
                }
                painter.drawPath(transform.map(path));
            }
            else if (args.size()==18 && args[0]==QString("PROJECT_OBJECT"))
            {
                calcGnomonicProjection(painter,args[1],args[2],exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]),exp(args[7]),
                        Vector3d(exp(args[8]),exp(args[9]),exp(args[10])),exp(args[11]),exp(args[12]),exp(args[13])
                        ,exp(args[14]),exp(args[15]),exp(args[16]),exp(args[17]));
            }
            else if(args.size()==7 && args[0]==QString("DRAW_GEAR"))
            {
                QPainterPath path;
                draw_gear(path,exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]));
                painter.drawPath(transform.map(path));
            }
            else
            {
                return Err(i,args[0]+QString(" : Commande inconnue ou mauvais nombre d'arguments (%1)").arg(args.size()));
            }
        }
    }

    return Err(0,"Pas d'erreur");
}
