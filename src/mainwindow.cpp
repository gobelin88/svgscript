﻿#include "mainwindow.h"

#include "displayer.h"

#include <complex>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    w_svg=new SvgView();

    version=QString("v6.1");

    pe=nullptr;

    te_script=new CodeEditor();
    te_console=new QTextEdit();

    highlighter_script=new Highlighter(te_script->document());
    highlighter_console=new Highlighter(te_console->document());

    pb_load=new QPushButton("Load",this);
    pb_save=new QPushButton("Save",this);
    pb_run=new QPushButton("Run (F1)",this);
    pb_search=new QPushButton("Search",this);
    pb_search->setShortcut(tr("Ctrl+F"));
    pb_save->setShortcut(tr("Ctrl+Shift+S"));
    pb_load->setShortcut(tr("Ctrl+O"));
    pb_run->setShortcut(tr("F1"));
    a_direct_save=new QAction(this);
    a_direct_save->setShortcut(tr("Ctrl+S"));
    this->addAction(a_direct_save);



    l_pb=new QHBoxLayout();
    l_pb->addWidget(pb_load);
    l_pb->addWidget(pb_save);
    l_pb->addWidget(pb_run );
    l_pb->addWidget(pb_search);

    QWidget * widget_slider=new QWidget;
    slider_layout=new QVBoxLayout(widget_slider);
    slider_layout->addStretch();

    w_tree=new QTreeWidget;

    tab_view=new QTabWidget();
    tab_view->addTab(widget_slider,"Sliders");
    tab_view->addTab(te_script,"Script");
    tab_view->addTab(w_tree,"Tree");

    QWidget * leftWidget=new QWidget;
    QVBoxLayout * vLayout=new QVBoxLayout(leftWidget);

    QSplitter * spliter_edit=new QSplitter(Qt::Vertical);

    spliter_edit->addWidget(tab_view);
    spliter_edit->addWidget(te_console);
    vLayout->addWidget(spliter_edit);
    vLayout->addLayout(l_pb);

    QSplitter * spliter=new QSplitter(Qt::Horizontal);
    spliter->addWidget(leftWidget);
    spliter->addWidget(w_svg);

    this->setCentralWidget(spliter);
    te_script->setBaseSize(400,600);
    w_svg->setBaseSize(400,600);
    //leftWidget->setMaximumWidth(512);
    //w_svg->setMinimumSize(600,600);
    //spliter->setMinimumSize(800,600);

    connect(pb_load,SIGNAL(clicked()),this,SLOT(slot_load()));
    connect(pb_save,SIGNAL(clicked()),this,SLOT(slot_save()));
    connect(pb_run,SIGNAL(clicked()),this,SLOT(initSliders()));
    connect(pb_run,SIGNAL(clicked()),this,SLOT(slot_run()));
    connect(a_direct_save,SIGNAL(triggered()),this,SLOT(slot_direct_save()));
    connect(te_script,SIGNAL(textChanged()),this,SLOT(slot_modified()));
    connect(pb_search,SIGNAL(clicked()),this,SLOT(search()));

    loadStyle(":/qss_script/rc/style.txt");
}

MainWindow::~MainWindow()
{

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
            removeSliders();

            te_script->setPlainText(QString(file.readAll()));
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

void MainWindow::removeSliders()
{
    for(int i=0;i<sliderlist.size();i++)
    {
        sliderlist[i]->close();
    }
}

void MainWindow::initSliders()
{
    for(int i=0;i<sliderlist.size();i++)
    {
        sliderlist[i]->reset();
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
    engine.globalObject().setProperty("$torad", M_PI/180.0);
    engine.globalObject().setProperty("$todeg", 180.0/M_PI);
    pe=&engine;

    QFileInfo info(current_file_path);
    QString raw_content=te_script->toPlainText();
    QStringList content=comment(raw_content).replace("$NAME",info.baseName()).split(";");
    QStringList content_lines=raw_content.split("\n");



    Err err=process(content);
    int index=0,err_line=-1;
    for(int k=0;k<content_lines.size();k++)
    {
        if(content_lines[k].contains(";"))
        {
            index++;
        }
        if(index==err.id)err_line=k;
    }

    if(err.id)
    {
        te_script->setError(err_line+1,true);
        te_console->append(QString("Erreur Ligne %1 : %2 \n").arg(err_line+2).arg(err.cmd));
        help(err.cmd);
        te_script->update();
    }
    else
    {
        te_script->setError(err_line,false);
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

double getEllipticStepL(double ra,double rb,double dL,int N,double dtheta)
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
        dt=dtheta*TO_RAD-theta;

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
        int n2,
        int mode,
        double offset,
        double theta0,
        double dtheta)
{
    if(n>=0 && n<3)return;
    if(rb<ra/10)return;
    if(ra<rb/10)return;
    if(ra<0)return;
    if(rb<0)return;

    double Le=getEllipsePerimetre(ra,rb)*dtheta/360.0;
    //Calcul du nombre de creneaux dans L
    int N= (n==-1) ? Le/(2.0*dL)-2.0 : n;

    double theta=theta0*TO_RAD+M_PI;

    double Lstep=getEllipticStepL(ra,rb,dL,N,dtheta);//Le/N;

    pts.moveTo(center+QPointF(-ra,0));
    for(int i=0;i<N;i++)
    {
        double step=getEllipticStep(ra,rb,theta,Lstep);

        double x1=ra*cos(theta),y1=rb*sin(theta);
        double x2=ra*cos(theta+step),y2=rb*sin(theta+step);
        theta+=step;

        QLineF line(center+QPointF(x1,y1),center+QPointF(x2,y2));

        draw_lineCreneaux(pts,line,E,dL,n2,mode,offset);
    }


    this->te_console->append(QString("DEFINE Lstep=%1  Nstep=%2").arg(Lstep).arg(N));
    pe->globalObject().setProperty("Lstep",Lstep);
    pe->globalObject().setProperty("Nstep",N);
}

void clip(QPainterPath & pts,QPointF & cp,const QPointF & u,const QPointF & v,double E,double dL)
{
    double ra=0.28, rb=1.2;//32 trop serré
    double dr=1.8,m=2*ra;

    cp+=v*E-u*ra;pts.lineTo(cp);
    cp+=v*rb+u*ra;pts.lineTo(cp);

    cp+=u*dr;pts.lineTo(cp);
    cp-=v*(rb+E);pts.lineTo(cp);
    cp+=v*(rb+E)+u*m;pts.lineTo(cp);
    cp+=u*(dL-2*(dr+m));pts.lineTo(cp);
    cp-=v*(rb+E)-u*m;pts.lineTo(cp);
    cp+=v*(rb+E);pts.lineTo(cp);
    cp+=u*dr;pts.lineTo(cp);

    cp-=v*rb-u*ra;pts.lineTo(cp);
    cp-=v*E+u*ra;pts.lineTo(cp);
}

void clipb(QPainterPath & pts,QPointF & cp,const QPointF & u,const QPointF & v,double E,double dL)
{
    double ra=0.28, rb=1.2;
    double dr=1.8,m=2*ra;

    cp+=v*E-u*ra;pts.lineTo(cp);
    cp+=v*rb+u*ra;pts.lineTo(cp);

    cp+=u*dr;pts.lineTo(cp);
    cp+=u*m;pts.lineTo(cp);
    cp+=u*(dL-2*(dr+m));pts.lineTo(cp);
    cp+=u*m;pts.lineTo(cp);
    cp+=u*dr;pts.lineTo(cp);

    cp-=v*rb-u*ra;pts.lineTo(cp);
    cp-=v*E+u*ra;pts.lineTo(cp);
}

void clipc(QPainterPath & pts,QPointF & cp,const QPointF & u,const QPointF & v,double E,double dL)
{
    double ra=0.28, rb=0;//32 trop serré
    double dr=1.8,m=2*ra;

    cp+=v*E-u*ra;pts.lineTo(cp);
    cp+=v*rb+u*ra;pts.lineTo(cp);

    cp+=u*dr;pts.lineTo(cp);
    cp-=v*(rb+E);pts.lineTo(cp);
    cp+=v*(rb+E)+u*m;pts.lineTo(cp);
    cp+=u*(dL-2*(dr+m));pts.lineTo(cp);
    cp-=v*(rb+E)-u*m;pts.lineTo(cp);
    cp+=v*(rb+E);pts.lineTo(cp);
    cp+=u*dr;pts.lineTo(cp);

    cp-=v*rb-u*ra;pts.lineTo(cp);
    cp-=v*E+u*ra;pts.lineTo(cp);
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
        int N= (n==-1) ? round(L/(2.0*dL))-2.0 : n;

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
    else if(mode==3 || mode==4)
    {
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());


        //Calcul du nombre de creneaux dans L
        int N= (n==-1) ? round(L/(2.0*dL))-2.0 : n;
        if(mode==4)N-=1;

        //Calcul du reste Epsilon
        double Eps=(mode==4)?(L-((N+1)*2.0*dL+dL))/2.0:(L-((N)*2.0*dL+dL))/2.0;

        //On dessine tout cela
        QPointF cp=line.p1();
        cp+=u*Eps+offset*u;pts.lineTo(cp);
        if(mode==4)
        {
            cp-=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
        }


        for(int i=0;i<N;i++)
        {
            clip(pts,cp,u,v,E,dL);
            cp+=u*dL;pts.lineTo(cp);
        }
        clip(pts,cp,u,v,E,dL);

        if(mode==4)
        {
            cp+=u*dL;pts.lineTo(cp);
            cp+=v*E;pts.lineTo(cp);
        }

        cp+=u*Eps-offset*u;pts.lineTo(cp);
    }
    else if(mode==5 || mode==6)
    {
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());


        //Calcul du nombre de creneaux dans L
        int N= (n==-1) ? round(L/(2.0*dL))-2.0 : n;
        if(mode==6)N-=1;

        //Calcul du reste Epsilon
        double Eps=(mode==6)?(L-((N+1)*2.0*dL+dL))/2.0:(L-((N)*2.0*dL+dL))/2.0;

        //On dessine tout cela
        QPointF cp=line.p1();
        cp+=u*Eps+offset*u;pts.lineTo(cp);
        if(mode==6)
        {
            cp-=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
        }


        for(int i=0;i<N;i++)
        {
            clipb(pts,cp,u,v,E,dL);
            cp+=u*dL;pts.lineTo(cp);
        }
        clipb(pts,cp,u,v,E,dL);

        if(mode==6)
        {
            cp+=u*dL;pts.lineTo(cp);
            cp+=v*E;pts.lineTo(cp);
        }

        cp+=u*Eps-offset*u;pts.lineTo(cp);
    }
    else if(mode==7 || mode==8)
    {
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());


        //Calcul du nombre de creneaux dans L
        int N= (n==-1) ? round(L/(2.0*dL))-2.0 : n;
        if(mode==8)N-=1;

        //Calcul du reste Epsilon
        double Eps=(mode==8)?(L-((N+1)*2.0*dL+dL))/2.0:(L-((N)*2.0*dL+dL))/2.0;

        //On dessine tout cela
        QPointF cp=line.p1();
        cp+=u*Eps+offset*u;pts.lineTo(cp);
        if(mode==8)
        {
            cp-=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
        }


        for(int i=0;i<N;i++)
        {
            clipc(pts,cp,u,v,E,dL);
            cp+=u*dL;pts.lineTo(cp);
        }
        clipc(pts,cp,u,v,E,dL);

        if(mode==8)
        {
            cp+=u*dL;pts.lineTo(cp);
            cp+=v*E;pts.lineTo(cp);
        }

        cp+=u*Eps-offset*u;pts.lineTo(cp);
    }
    else if(mode==9)
    {
        double L=line.length();

        //Base
        QPointF u(line.dx()/L,line.dy()/L);
        QPointF v(u.y(),-u.x());
        if(mode==2){v=-v;}

        //Calcul du nombre de creneaux dans L
        int N= (n==-1) ? round(L/(2.0*dL))-2.0 : n;

        //Calcul du reste Epsilon
        double Eps=(L-(N*2.0*dL+dL))/2.0;

        //On dessine tout cela
        QPointF cp=line.p1();
        cp+=u*Eps+offset*u;pts.moveTo(cp);

        for(int i=0;i<N;i++)
        {
            pts.moveTo(cp);
            cp+=v*E;pts.lineTo(cp);
            cp+=u*dL;pts.lineTo(cp);
            cp-=v*E;pts.lineTo(cp);
            cp-=u*dL;pts.lineTo(cp);

            cp+=2*u*dL;
        }

        pts.moveTo(cp);
        cp+=v*E;pts.lineTo(cp);
        cp+=u*dL;pts.lineTo(cp);
        cp-=v*E;pts.lineTo(cp);
        cp-=u*dL;pts.lineTo(cp);

        cp+=u*dL;
        cp+=u*Eps-offset*u;pts.moveTo(cp);
    }
    else if(mode==10 || mode==11)
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

QPolygonF makePolygon(QVector<QLineF> list_lines)
{
    QVector<QPointF> list_points;
    for(int i=0;i<list_lines.size();i++)
    {
        list_points.append(list_lines[i].p2());
    }
    return QPolygonF (list_points);
}

QPainterPath getPuzzleShape(QLineF line,int mode)
{
    double L=line.length();
    QPointF u(line.dx(),line.dy());
    QPointF v(u.y(),-u.x());
    QPointF center=(line.p1()+line.p2())/2.0;
    QPainterPath path;

    if(mode==0 || mode==1)
    {
        if(mode==0)
        {
            v=-v;
        }

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
    }
    else if(mode==2 || mode==3)
    {
        v=v*(1.0/L);

        if(mode==2)
        {
            v=-v;
        }

        QPointF p=line.p1(); path.moveTo(p);
        p+=u/3;              path.lineTo(p);
        p+=v*2.5;            path.lineTo(p);
        p+=u/3;              path.lineTo(p);
        p-=v*2.5;            path.lineTo(p);
        p+=u/3;              path.lineTo(p);
    }

    return path;
}

QPainterPath MainWindow::draw_circle_tangent(QLineF line,double alpha)
{
    QPainterPath path;
    QPointF A=line.p1();
    QPointF B=line.p2();

    if(alpha>0)
    {
        double L=line.length();
        double r=L/(2*sin(alpha*TO_RAD));

        QPointF u(line.dx(),line.dy());
        QPointF v(u.y(),-u.x());
        QPointF center=(line.p1()+line.p2())/2.0;

        double y=(L)/2.0;
        double x=sqrt(r*r-y*y);
        double dtheta=2*atan(y/x)*TO_DEG;

        QPointF C=center+v/L*x;
        QPointF V=(A-C)/r;
        double theta0=atan2(-V.y(),V.x())*TO_DEG;

        path.moveTo(A);
        path.arcTo(C.x()-r,C.y()-r,2*r,2*r,theta0, dtheta);

        this->te_console->append(QString("DRAW_TANGENT_CIRCLE --> DEFINE Cx=%1  Cy=%2  Ra=%3 theta0=%4 dtheta=%5").arg(C.x()).arg(C.y()).arg(r).arg(theta0).arg(dtheta));
        pe->globalObject().setProperty("Cx",C.x());
        pe->globalObject().setProperty("Cy",C.y());
        pe->globalObject().setProperty("Ra",r);
        pe->globalObject().setProperty("theta0",theta0);
        pe->globalObject().setProperty("dtheta",dtheta);
    }
    else
    {
        path.moveTo(A);
        path.lineTo(B);
    }

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

    QPointF C1=center+v/L*x;
    QPointF C2=center-v/L*x;

    //path.addEllipse(C1.x()-r,C1.y()-r,2*r,2*r);
    //path.addEllipse(C2.x()-r,C2.y()-r,2*r,2*r);

    double dtheta=2*atan(y/x)*180/M_PI;

    QPointF V1=(A-C1)/r;
    QPointF V2=(B-C2)/r;

    path.moveTo(A);


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

void arcToN(QPainterPath & path,QRectF rect,double a,double b,int n)
{
    double rx=rect.width()/2;
    double ry=rect.height()/2;
    double cx=rect.x()+rx,cy=rect.y()+ry;

    for(int k=0;k<n;k++)
    {
        double alpha=-(a+b*k/n)*TO_RAD;
        path.lineTo(cx+rx*cos(alpha),cy+ry*sin(alpha));
    }
}

QPainterPath roundRectPath(const QRectF &rect,double radius)
{
    double diam = 2 * radius;

    double x1, y1, x2, y2;
    rect.getCoords(&x1, &y1, &x2, &y2);

    int N=20;
    QPainterPath path;
    path.moveTo(x2, y1 + radius);
    //path.arcTo(QRectF(x2 - diam, y1, diam, diam), 0.0, +90.0);
    arcToN(path,QRectF(x2 - diam, y1, diam, diam), 0.0, +90.0,N);
    path.lineTo(x1 + radius, y1);
    //path.arcTo(QRectF(x1, y1, diam, diam), 90.0, +90.0);
    arcToN(path,QRectF(x1, y1, diam, diam),90, +90.0,N);
    path.lineTo(x1, y2 - radius);
    //path.arcTo(QRectF(x1, y2 - diam, diam, diam), 180.0, +90.0);
    arcToN(path,QRectF(x1, y2 - diam, diam, diam), 180.0, +90.0,N);
    path.lineTo(x1 + radius, y2);
    //path.arcTo(QRectF(x2 - diam, y2 - diam, diam, diam), 270.0, +90.0);
    arcToN(path,QRectF(x2 - diam, y2 - diam, diam, diam), 270.0, +90.0,N);
    path.closeSubpath();
    return path;
}

bool MainWindow::calcProjection(QPainterPath & path,
                                QString obj_filename,
                                QPointF c,
                                Vector3d euler_angles,
                                float scale,int mode)
{
    Object pobj(obj_filename,(double)scale,euler_angles);
    if(!pobj.isOpen()){return false;}

    pobj.drawProj(path,c,mode);

    return true;
}

bool MainWindow::calcGnomonicProjection(QPainter & painter,
                                        QString obj_filename,
                                        QString map_filename,
                                        int res,
                                        float scale,
                                        bool meridiens,
                                        int Nlat,
                                        int Nlon,
                                        Vector3d euler_angles,
                                        double radius,int idpN,int idpS,double W,double dL,double dA,double marge,int mode)
{
    Object * pobj=new Object(obj_filename,scale,euler_angles);
    if(!pobj->isOpen()){return false;}

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

    pobj->getGnomonicAll(map,res,painter,meridiens,Nlat,Nlon,radius,idpN,idpS,W,dL,dA,marge,mode);
    std::cout<<std::endl;

    displayer->add(pobj);
    return true;
}

void MainWindow::draw_gear(QPainterPath & path,double m,int n,double alpha,double daxe,int nb_spokes)
{
    double dp=m*n;                      //Diametre primitif
    double db=dp*cos(alpha*TO_RAD);     //Diamètre de base
    double p=M_PI*m;                    //Pas primitif
    double pb=p*cos(alpha*TO_RAD);      //Pas de base
    double df=dp-2*m;                 //Diamètre de pied
    double dh=dp+2*m;                   //Diamètre de tête
    double t=sqrt((dp/db)*(dp/db)-1);                        //
    double rho=0.0;                                          //
    double delta_p=(sin(t)-t*cos(t));                        //
    double dalpha= p*180/(M_PI*dp)*0.5 + delta_p*180/(M_PI); //

    //if(db<df)db=df;

    QTransform rt,rt2;
    rt.rotate( -dalpha );
    path.moveTo( rt.map(QPointF(df*0.5,0)) );

    for(int k=0;k<n;k++)
    {
        rt.reset();
        rt.rotate(  k*360.0/n-dalpha);

        rt2.reset();
        rt2.rotate( k*360.0/n+dalpha);

        path.lineTo( rt.map(QPointF(df*0.5,0)) );
        path.lineTo( rt.map(QPointF(db*0.5,0)) );


        rho=0.0;
        t=0.0;

        double te=sqrt((dh/db)*(dh/db)-1);
        do
        {
            double X=(cos(t)+t*sin(t))*db*0.5;
            double Y=(sin(t)-t*cos(t))*db*0.5;
            rho=sqrt(X*X+Y*Y);
            path.lineTo( rt.map(QPointF(X,Y)) );
            t+=te*0.1;
        }
        while(rho<dh*0.5-te*0.1*0.5);

        do
        {
            t-=te*0.1;
            double X=(cos(t)+t*sin(t))*db*0.5;
            double Y=-(sin(t)-t*cos(t))*db*0.5;
            rho=sqrt(X*X+Y*Y);
            path.lineTo( rt2.map(QPointF(X,Y)) );
        }
        while(t>te*0.1*0.5);
        path.lineTo( rt2.map(QPointF(df*0.5,0)) );
    }

    rt2.reset();
    rt2.rotate( -dalpha );

    path.lineTo( rt2.map( QPointF(df*0.5,0) ) );

    this->te_console->append("------------------------");
    pe->globalObject().setProperty("Dp", dp);
    pe->globalObject().setProperty("Dh", dh);
    pe->globalObject().setProperty("Df", df);
    pe->globalObject().setProperty("Db", db);
    pe->globalObject().setProperty("Pb", pb);
    pe->globalObject().setProperty("Pp", p);
    this->te_console->append(QString("DEFINE Dp=%1").arg(dp));
    this->te_console->append(QString("DEFINE Dh=%1").arg(dh));
    this->te_console->append(QString("DEFINE Df=%1").arg(df));
    this->te_console->append(QString("DEFINE Db=%1").arg(db));
    this->te_console->append(QString("DEFINE Pb=%1").arg(pb));
    this->te_console->append(QString("DEFINE Pp=%1").arg(p));

    path.moveTo(0,0);
    path.addEllipse(-daxe/2,-daxe/2,daxe,daxe);

    //    double rs=(df-daxe)/4;
    //    double rd=(rs+0.5*daxe);
    //    double A=2*(rs+0.5*daxe)*sin(M_PI/nb_spokes);
    //    double ra=A-(df-daxe)*0.25;

    //    for(int k=0;k<nb_spokes;k++)
    //    {
    //        double alpha=2*M_PI/nb_spokes*k;

    //        path.moveTo(x+rd*cos(alpha),y+rd*sin(alpha));
    //        path.addEllipse(
    //                    x+rd*cos(alpha)-ra,
    //                    y+rd*sin(alpha)-ra,
    //                    ra*2,ra*2);
    //    }

    double sm=df*0.1;
    for(int k=0;k<nb_spokes;k++)
    {
        double alpha=2*M_PI/nb_spokes*k;
        double beta=2*M_PI/nb_spokes*(k+1);

        QPointF A((daxe/2+sm)*cos(alpha),(daxe/2+sm)*sin(alpha));
        QPointF B((df  /2-sm)*cos(alpha),(df  /2-sm)*sin(alpha));

        QPointF C((daxe/2+sm)*cos(beta),(daxe/2+sm)*sin(beta));
        QPointF D((df  /2-sm)*cos(beta),(df  /2-sm)*sin(beta));

        QPointF U1=B-A;
        U1=U1/sqrt(QPointF::dotProduct(U1,U1));
        QPointF V(-U1.y(),U1.x());

        QPointF U2=D-C;
        U2=U2/sqrt(QPointF::dotProduct(U2,U2));
        QPointF W(-U2.y(),U2.x());

        QVector2D PB=QVector2D(B+V*sm/2);
        QVector2D PD=QVector2D(D-W*sm/2);

        QVector2D PA=QVector2D(A+V*sm/2);
        QVector2D PC=QVector2D(C-W*sm/2);

        double angleBD=TO_DEG*acos(QVector2D::dotProduct(PB,PD)/(PD.length()*PB.length()));
        double rhoBD=PB.length();
        double sma_BD=atan((sm/2.0)/(df/2.0-sm))*TO_DEG;

        double angleAC=TO_DEG*acos(QVector2D::dotProduct(PA,PC)/(PA.length()*PC.length()));
        double rhoAC=PA.length();
        double sma_AC=atan((sm/2.0)/(daxe/2.0+sm))*TO_DEG;

        path.moveTo(A+V*sm/2);
        path.lineTo(B+V*sm/2);
        path.arcTo(-rhoBD,-rhoBD,rhoBD*2,rhoBD*2,-360/nb_spokes*k-sma_BD,-angleBD);
        //path.lineTo(D-W*sm/2);
        path.lineTo(C-W*sm/2);
        path.arcTo(-rhoAC,-rhoAC,rhoAC*2,rhoAC*2,-360/nb_spokes*k-sma_AC-angleAC,angleAC);
        //path.lineTo(A+V*sm/2);
    }

}

void MainWindow::draw_gear_s(QPainterPath & path,double m,int n,double daxe,int nb_spokes)
{
    double dp=m*n;                      //Diametre primitif
    double df=dp-2*m;                   //Diamètre de pied
    double dh=dp+2*m;                   //Diamètre de tête

    QTransform rt;
    path.moveTo( rt.map(QPointF(df*0.5,0)) );

    for(int k=0;k<n;k++)
    {
        rt.reset();
        rt.rotate( k*360.0/n );

        path.lineTo( rt.map(QPointF(dh*0.5,0)) );
        path.lineTo( rt.map(QPointF(df*0.5,0)) );
    }
    rt.reset();
    rt.rotate( 360.0 );
    path.lineTo( rt.map( QPointF(dh*0.5,0) ) );

    this->te_console->append("------------------------");
    pe->globalObject().setProperty("Dp", dp);
    pe->globalObject().setProperty("Dh", dh);
    pe->globalObject().setProperty("Df", df);
    this->te_console->append(QString("DEFINE Dp=%1").arg(dp));
    this->te_console->append(QString("DEFINE Dh=%1").arg(dh));
    this->te_console->append(QString("DEFINE Df=%1").arg(df));

    path.moveTo(0,0);
    path.addEllipse(-daxe/2,-daxe/2,daxe,daxe);

    //    double rs=(df-daxe)/4;
    //    double rd=(rs+0.5*daxe);
    //    double A=2*(rs+0.5*daxe)*sin(M_PI/nb_spokes);
    //    double ra=A-(df-daxe)*0.25;

    //    for(int k=0;k<nb_spokes;k++)
    //    {
    //        double alpha=2*M_PI/nb_spokes*k;

    //        path.moveTo(x+rd*cos(alpha),y+rd*sin(alpha));
    //        path.addEllipse(
    //                    x+rd*cos(alpha)-ra,
    //                    y+rd*sin(alpha)-ra,
    //                    ra*2,ra*2);
    //    }

    double sm=df*0.1;
    for(int k=0;k<nb_spokes;k++)
    {
        double alpha=2*M_PI/nb_spokes*k;
        double beta=2*M_PI/nb_spokes*(k+1);

        QPointF A((daxe/2+sm)*cos(alpha),(daxe/2+sm)*sin(alpha));
        QPointF B((df  /2-sm)*cos(alpha),(df  /2-sm)*sin(alpha));

        QPointF C((daxe/2+sm)*cos(beta),(daxe/2+sm)*sin(beta));
        QPointF D((df  /2-sm)*cos(beta),(df  /2-sm)*sin(beta));

        QPointF U1=B-A;
        U1=U1/sqrt(QPointF::dotProduct(U1,U1));
        QPointF V(-U1.y(),U1.x());

        QPointF U2=D-C;
        U2=U2/sqrt(QPointF::dotProduct(U2,U2));
        QPointF W(-U2.y(),U2.x());

        QVector2D PB=QVector2D(B+V*sm/2);
        QVector2D PD=QVector2D(D-W*sm/2);

        QVector2D PA=QVector2D(A+V*sm/2);
        QVector2D PC=QVector2D(C-W*sm/2);

        double angleBD=TO_DEG*acos(QVector2D::dotProduct(PB,PD)/(PD.length()*PB.length()));
        double rhoBD=PB.length();
        double sma_BD=atan((sm/2.0)/(df/2.0-sm))*TO_DEG;

        double angleAC=TO_DEG*acos(QVector2D::dotProduct(PA,PC)/(PA.length()*PC.length()));
        double rhoAC=PA.length();
        double sma_AC=atan((sm/2.0)/(daxe/2.0+sm))*TO_DEG;

        path.moveTo(A+V*sm/2);
        path.lineTo(B+V*sm/2);
        path.arcTo(-rhoBD,-rhoBD,rhoBD*2,rhoBD*2,-360/nb_spokes*k-sma_BD,-angleBD);
        //path.lineTo(D-W*sm/2);
        path.lineTo(C-W*sm/2);
        path.arcTo(-rhoAC,-rhoAC,rhoAC*2,rhoAC*2,-360/nb_spokes*k-sma_AC-angleAC,angleAC);
        //path.lineTo(A+V*sm/2);
    }

}

void MainWindow::draw_pendule(QPainterPath & path,double x,double y,double P,double theta,double daxe1,double daxe2)
{
    //Calcule de la longueur du pendule
    double P0=P/(1+theta*theta/16.0*M_PI*M_PI/180/180);
    double L=P0*P0*9.81/(4*M_PI*M_PI)*1e3;

    QPointF c(x,y);

    double W=5;
    double alpha_1=asin(W/daxe1)*180/M_PI;
    double alpha_2=asin(W/daxe2)*180/M_PI;

    std::cout<<alpha_1<<" "<<alpha_2<<std::endl;

    path.moveTo(c+QPointF(daxe1/2*sin(alpha_1/180*M_PI),daxe1/2*cos(alpha_1/180*M_PI)));
    path.arcTo(QRectF(c.x()-daxe1/2,c.y()-daxe1/2,daxe1,daxe1),alpha_1 -90,360-alpha_1*2);

    path.arcTo(QRectF(c.x()-daxe2/2,c.y()-daxe2/2+L,daxe2,daxe2),alpha_2 +90,360-alpha_2*2);
    path.lineTo(c+QPointF(daxe1/2*sin(alpha_1/180*M_PI),daxe1/2*cos(alpha_1/180*M_PI)));

    pe->globalObject().setProperty("Lp", L);
    this->te_console->append(QString("DEFINE Lp=%1").arg(L));
}

QPointF sym(double df,double dh,QPointF p)
{
    double na=sqrt(p.x()*p.x()+p.y()*p.y());
    double nb=dh*0.5-(na-df*0.5);

    if(nb>dh*0.5)
    {
        nb=dh*0.5;
    }

    return QPointF(p.x()*nb/na,p.y()*nb/na);
}

void MainWindow::draw_gear_r(QPainterPath & path, double m, int n, double alpha,double e)
{
    double dp=m*n;                                          //Diametre primitif
    double db=dp*cos(alpha*TO_RAD);                         //Diamètre de base
    double p=M_PI*m;                                        //Pas primitif
    double pb=p*cos(alpha*TO_RAD);                          //Pas de base
    double df=dp-2*m;                                       //Diamètre de pied
    double dh=dp+2*m;                                       //Diamètre de tête
    double t=sqrt((dp/db)*(dp/db)-1);                        //
    double rho=0.0;                                          //
    double delta_p=(sin(t)-t*cos(t));                        //
    double dalpha= p*180/(M_PI*dp)*0.5 + delta_p*180/(M_PI); //
    double daxe=dh+e;

    QTransform rt,rt2;
    rt.rotate( -dalpha );

    path.moveTo( rt.map(sym(df,dh, QPointF(df*0.5,0))) );

    for(int k=0;k<n;k++)
    {
        rt.reset();
        rt.rotate(  k*360.0/n-dalpha);

        rt2.reset();
        rt2.rotate( k*360.0/n+dalpha);

        path.lineTo( rt.map(sym(df,dh,QPointF(df*0.5,0))) );
        path.lineTo( rt.map(sym(df,dh,QPointF(db*0.5,0))) );

        rho=0.0;
        t=0.0;

        double te=sqrt((dh/db)*(dh/db)-1);
        do
        {
            double X=(cos(t)+t*sin(t))*db*0.5;
            double Y=(sin(t)-t*cos(t))*db*0.5;
            rho=sqrt(X*X+Y*Y);
            path.lineTo( rt.map(sym(df,dh,QPointF(X,Y))) );
            t+=te*0.1;
        }
        while(rho<dh*0.5-te*0.1*0.5);

        do
        {
            t-=te*0.1;
            double X=(cos(t)+t*sin(t))*db*0.5;
            double Y=-(sin(t)-t*cos(t))*db*0.5;
            rho=sqrt(X*X+Y*Y);
            path.lineTo( rt2.map(sym(df,dh,QPointF(X,Y))) );
        }
        while(t>te*0.1*0.5);
        path.lineTo( rt2.map(sym(df,dh,QPointF(df*0.5,0))) );
    }

    rt2.reset();
    rt2.rotate( -dalpha );

    path.lineTo( rt2.map( sym(df,dh,QPointF(df*0.5,0)) ) );

    if(daxe>0)
    {
        path.moveTo(0,0);
        path.addEllipse(-(daxe)/2,-(daxe)/2,(daxe),(daxe));
    }

    this->te_console->append("------------------------");
    pe->globalObject().setProperty("Dp", dp);
    pe->globalObject().setProperty("Dh", dh);
    pe->globalObject().setProperty("Df", df);
    pe->globalObject().setProperty("Db", db);
    pe->globalObject().setProperty("Pb", pb);
    pe->globalObject().setProperty("Pp", p);
    this->te_console->append(QString("DEFINE Dp=%1").arg(dp));
    this->te_console->append(QString("DEFINE Dh=%1").arg(dh));
    this->te_console->append(QString("DEFINE Df=%1").arg(df));
    this->te_console->append(QString("DEFINE Db=%1").arg(db));
    this->te_console->append(QString("DEFINE Pb=%1").arg(pb));
    this->te_console->append(QString("DEFINE Pp=%1").arg(p));

}

void MainWindow::help(QString cmd)
{
    bool found=false;
    for(int i=0;i<NB_CMD;i++)
    {
        if(CommandsList[i].keyword.contains(cmd) || cmd=="all")
        {
            te_console->append(QString("%1 %2\n").arg(CommandsList[i].keyword).arg(CommandsList[i].help));
            found=true;
        }
    }
    if(!found)
    {
        te_console->append(QString("%1 : Not Found").arg(cmd));
    }
}

QColor getMoyPix(QRect rect,QImage & img)
{
    unsigned int pix_r=0;
    unsigned int pix_g=0;
    unsigned int pix_b=0;

    for(int i=rect.x();i<rect.x()+rect.width();i++)
    {
        for(int j=rect.y();j<rect.y()+rect.height();j++)
        {
            QRgb pix=img.pixel(i,j);
            pix_r+=qRed(pix);
            pix_g+=qGreen(pix);
            pix_b+=qBlue(pix);
        }
    }
    unsigned int s=(rect.width()*rect.height());
    return QColor(std::round(double(pix_r)/s),
                  std::round(double(pix_g)/s),
                  std::round(double(pix_b)/s));
}

QColor nearest(QColor c, std::vector<QColor> & list)
{
    unsigned int dist=255*255*3;
    QColor nearest_color=list[0];

    for(unsigned int i=0;i<list.size();i++)
    {
        int dr=list[i].red()-c.red();
        int dg=list[i].green()-c.green();
        int db=list[i].blue()-c.blue();
        int cdist=dr*dr+dg*dg+db*db;

        if( cdist<dist )
        {
            dist=cdist;
            nearest_color=list[i];
        }
    }
    return nearest_color;
}

QStringList MainWindow::loadScript(QString filename,bool & ok)
{
    QFile file(filename);
    QStringList content;
    ok=false;
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        ok=true;
        QFileInfo info(current_file_path);
        content=comment(file.readAll()).replace("$NAME",info.baseName()).split(";");
    }

    return content;
}

Err MainWindow::sub_process(QStringList content,QSvgGenerator & generator,QPainter & painter,DrawTree & drawtree)
{
    for(int i=0;i<content.size();i++)
    {
        QStringList args=content[i].split(QRegExp(" |\n"),QString::SkipEmptyParts);

        if(args.size()>0)
        {
            //te_console->append(QString("(%1,%2)=%3 :").arg(i).arg(args.size()).arg(args[0].toLocal8Bit().data()));

            if(args.size()==3 && args[0]==QString("SVG_BEGIN"))//Ok
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
            }
            else if(args.size()==1 && args[0]==QString("SVG_END"))//Ok
            {
                drawtree.draw(painter);
                drawtree.populate(w_tree);
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

                drawtree.setPen(pen);
            }
            else if(args.size()==5 && args[0]==QString("SVG_BRUSH"))//Ok
            {
                QBrush brush;
                brush.setColor( QColor(exp(args[1]),exp(args[2]),exp(args[3])) );
                if(args[4]==QString("None")){brush.setStyle(Qt::NoBrush);}
                else if (args[4]==QString("Solid")){brush.setStyle(Qt::SolidPattern);}
                else {return Err(i,args[0]+QString(" : Type inconnu"));}

                drawtree.setBrush(brush);
            }
            else if((args.size()==9 || args.size()==11) && args[0]==QString("DRAW_CIRCLE_CRENEAUX"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double r=exp(args[3]);

                path.moveTo(c+QPointF(r,0));

                if(args.size()==9)
                {
                    draw_ellipseCreneaux(path,c,r,r,exp(args[4]),exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]));
                }
                else
                {
                    draw_ellipseCreneaux(path,c,r,r,exp(args[4]),exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]),0,exp(args[9]),exp(args[10]));
                }

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==10 && args[0]==QString("DRAW_ELLIPSE_CRENEAUX"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double ra=exp(args[3]),rb=exp(args[4]);

                path.moveTo(c+QPointF(ra,0));
                draw_ellipseCreneaux(path,c,ra,rb,exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]),exp(args[9]),0);

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()>=4 && args[0]==QString("DRAW_CIRCLE"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double r=exp(args[3]);

                if(args.size()==4){path.moveTo(c+QPointF(r,0)); path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),0 ,360);}
                else if(args.size()==5){path.moveTo(c+QPointF(r*cos(exp(args[4])*TO_RAD),-r*sin(exp(args[4])*TO_RAD)));path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),exp(args[4]) ,360);}
                else if(args.size()==6){path.moveTo(c+QPointF(r*cos(exp(args[4])*TO_RAD),-r*sin(exp(args[4])*TO_RAD)));path.arcTo(QRectF(c.x()-r,c.y()-r,2*r,2*r),exp(args[4]) ,exp(args[5]));}

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()>=5 && args[0]==QString("DRAW_ELLIPSE"))//Ok
            {
                QPainterPath path;
                QPointF c= QPointF(exp(args[1]),exp(args[2]));
                double ra=exp(args[3]),rb=exp(args[4]);

                if(args.size()==5){path.moveTo(c+QPointF(ra,0));path.arcTo(QRectF(c.x()-ra,c.y()-rb,2*ra,2*rb),0 ,360);}
                else if(args.size()==6){path.moveTo(c+QPointF(ra*cos(exp(args[5])*TO_RAD),-rb*sin(exp(args[5])*TO_RAD)));path.arcTo(QRectF(c.x()-ra,c.y()-rb,2*ra,2*rb),exp(args[5]) ,360);}
                else if(args.size()==7){path.moveTo(c+QPointF(ra*cos(exp(args[5])*TO_RAD),-rb*sin(exp(args[5])*TO_RAD)));path.arcTo(QRectF(c.x()-ra,c.y()-rb,2*ra,2*rb),exp(args[5]) ,exp(args[6]));}

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==5 && args[0]==QString("DRAW_CROSS"))
            {
                QPointF c(exp(args[1]),exp(args[2]));
                double W=exp(args[3]),L=exp(args[4]);

                double A=W/2,B=(L-W)/2;

                QPainterPath path;

                path.moveTo(QPointF(c.x()-A,c.y()-A));
                path.lineTo(QPointF(c.x()-A-B,c.y()-A));
                path.lineTo(QPointF(c.x()-A-B,c.y()+A));
                path.lineTo(QPointF(c.x()-A,c.y()+A));
                path.lineTo(QPointF(c.x()-A,c.y()+A+B));
                path.lineTo(QPointF(c.x()+A,c.y()+A+B));
                path.lineTo(QPointF(c.x()+A,c.y()+A));
                path.lineTo(QPointF(c.x()+A+B,c.y()+A));
                path.lineTo(QPointF(c.x()+A+B,c.y()-A));
                path.lineTo(QPointF(c.x()+A,c.y()-A));
                path.lineTo(QPointF(c.x()+A,c.y()-A-B));
                path.lineTo(QPointF(c.x()-A,c.y()-A-B));
                path.lineTo(QPointF(c.x()-A,c.y()-A));

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==7 && args[0]==QString("DRAW_LINE_ROSACE"))//Ok
            {
                QPainterPath path=getRosaceElement(QLineF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])),exp(args[5]),exp(args[6]));
                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==5 && args[0]==QString("DRAW_LINE"))//Ok
            {
                QPainterPath path;
                path.moveTo( QPointF(exp(args[1]),exp(args[2])) );
                path.lineTo( QPointF(exp(args[3]),exp(args[4])) );
                drawtree.addNode(path,args[0]);
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
                    else if(args[k]=="Cl" && (k+6)<args.size())
                    {
                        QPointF P1 (exp(args[k+1]),exp(args[k+2]));
                        QPointF P2 (exp(args[k+3]),exp(args[k+4]));

                        QLineF line(P1,P2);
                        double E=exp(args[k+5]);
                        double dL=line.length();
                        int mode=exp(args[k+6]);

                        QPointF u(line.dx()/dL,line.dy()/dL);
                        QPointF v(u.y(),-u.x());

                        QPointF cp=P1;
                        if(mode==0)
                        {
                            clip(path,cp,u,v,E,dL);
                        }
                        else if(mode==1)
                        {
                            clipb(path,cp,u,v,E,dL);
                        }
                        if(mode==2)
                        {
                            clip(path,cp,u,-v,E,dL);
                        }
                        else if(mode==3)
                        {
                            clipb(path,cp,u,-v,E,dL);
                        }
                        if(mode==4)
                        {
                            clipc(path,cp,u,v,E,dL);
                        }
                        else if(mode==5)
                        {
                            clipc(path,cp,u,-v,E,dL);
                        }


                        k+=7;
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
                drawtree.addNode(path,args[0]);
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
                    else if(args[k]=="Lc")
                    {
                        draw_lineCreneaux(path,QLineF(P,P+QPointF(exp(args[k+1]),exp(args[k+2])) ),
                                exp(args[k+3]),exp(args[k+4]),exp(args[k+5]),exp(args[k+6]) );
                        P=P+QPointF(exp(args[k+1]),exp(args[k+2]));
                        k+=7;
                    }
                    else if(args[k]=="Cl" && (k+4)<args.size())
                    {
                        QPointF cp=P;
                        P+=QPointF(exp(args[k+1]),exp(args[k+2]));

                        QLineF line(cp,P);
                        double E=exp(args[k+3]);
                        double dL=line.length();
                        int mode=exp(args[k+4]);

                        QPointF u(line.dx()/dL,line.dy()/dL);
                        QPointF v(u.y(),-u.x());

                        if(mode==0)
                        {
                            clip(path,cp,u,v,E,dL);
                        }
                        else if(mode==1)
                        {
                            clipb(path,cp,u,v,E,dL);
                        }
                        else if(mode==2)
                        {
                            clipc(path,cp,u,v,E,dL);
                        }
                        else if(mode==3)
                        {
                            clip(path,cp,u,-v,E,dL);
                        }
                        else if(mode==4)
                        {
                            clipb(path,cp,u,-v,E,dL);
                        }
                        else if(mode==5)
                        {
                            clipc(path,cp,u,-v,E,dL);
                        }


                        k+=5;
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
                drawtree.addNode(path,args[0]);
            }
            else if(args.size()>=6 && args[0]==QString("DRAW_POLYGON"))//Ok
            {
                QPointF c=QPointF(exp(args[1]),exp(args[2]));
                double R=exp(args[3]);
                int n=exp(args[4]);
                double dt=2*M_PI/n;
                double t=exp(args[5])*M_PI/180.0;

                QPainterPath path;
                for(int j=0;j<=n;j+=1)
                {
                    if(j==0)
                    {
                        path.moveTo( c+R*QPointF(cos(j*dt+t),sin(j*dt+t)) );
                    }
                    else if (j==n)
                    {
                        path.lineTo( c+R*QPointF(cos(t),sin(t)) );
                    }
                    else
                    {
                        path.lineTo( c+R*QPointF(cos(j*dt+t),sin(j*dt+t)) );
                    }
                }

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()>=2 && args[0]==QString("DISP"))//Ok
            {
                QString str;
                for(int i=1;i<args.size();i++)
                {
                    str+=QString("%1=%2 ").arg(args[i]).arg(exp(args[i]));
                }
                te_console->append(str);
            }
            else if(args.size()>=2 && args[0]==QString("DISPTEXT"))//Ok
            {
                if(args.size()==5)
                {
                    QString str=args[1].replace("#"," ").arg(exp(args[2])).arg(exp(args[3])).arg(exp(args[4]));
                    te_console->append(str);
                }
                if(args.size()==4)
                {
                    QString str=args[1].replace("#"," ").arg(exp(args[2])).arg(exp(args[3]));
                    te_console->append(str);
                }
                if(args.size()==3)
                {
                    QString str=args[1].replace("#"," ").arg(exp(args[2]));
                    te_console->append(str);
                }
                else
                {
                    QString str=args[1];
                    te_console->append(str.replace("#"," "));
                }
            }
            else if(args.size()>=2 && args[0]==QString("SAVE_CONSOLE"))//Ok
            {
                QFile file(args[1]);
                if(file.open(QIODevice::Text | QIODevice::WriteOnly))
                {
                    file.write(te_console->toPlainText().toUtf8());
                    file.close();
                }
                else
                {
                    QMessageBox::critical(this,"Error","Unable to save console");
                }
            }

            else if(args.size()>=2 && args[0]==QString("DISPCSV"))//Ok
            {
                QString str;
                for(int i=1;i<args.size();i++)
                {
                    str+=QString("%1;").arg(exp(args[i]));
                }
                te_console->append(str);
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
                    slider=new MyQSlider(args[1],exp(args[3]),exp(args[4]),exp(args[2]),this);
                    sliderlist.append(slider);
                    connect(slider,SIGNAL(deleted(QString)),this,SLOT(removeSlider(QString)));
                    slider_layout->addWidget(slider);

                    pe->globalObject().setProperty(slider->name(),slider->get_value());
                }

                pe->globalObject().setProperty(slider->name(),slider->get_value());
            }
            else if(args.size()==3 && args[0]==QString("DEFINE"))//Ok
            {
                pe->globalObject().setProperty(args[1], exp(args[2]));
            }
            else if(args.size()==2 && args[0]==QString("ENTITY"))//Ok
            {
                drawtree.addEntity(args[1]);
            }
            else if(args.size()==2 && args[0]==QString("ENTITY_REMOVE"))//Ok
            {
                drawtree.removeEntity(args[1]);
            }
            else if(args.size()==4 && args[0]==QString("ENTITY_COPY"))//Ok
            {
                QTransform t;
                t.translate(exp(args[2]),exp(args[3]));
                drawtree.copyEntity(args[1],t);
            }
            else if(args.size()==4 && args[0]==QString("ENTITY_TRANSLATE"))//Ok
            {
                QTransform t;
                t.translate(exp(args[2]),exp(args[3]));
                drawtree.moveEntity(args[1],t);
            }
            else if(args.size()==4 && args[0]==QString("ENTITY_ROTATE"))//Ok
            {
                QTransform t;
                t.translate(exp(args[2]),exp(args[3]));
                t.rotate(exp(args[4]));
                t.translate(-exp(args[2]),-exp(args[3]));
                drawtree.moveEntity(args[1],t);
            }
            else if(args.size()==6 && args[0]==QString("ENTITY_SCALE"))//Ok
            {
                QTransform t;
                t.translate(exp(args[2]),exp(args[3]));
                t.scale(exp(args[4]),exp(args[5]));
                t.translate(-exp(args[2]),-exp(args[3]));
                drawtree.moveEntity(args[1],t);
            }

            else if((args.size()==2 || args.size()==3) && args[0]==QString("ENTITY_BOUNDING_RECT"))//Ok
            {
                Entity * ptr=drawtree.findEntity(args[1]);
                if(ptr)
                {
                    if(args.size()==2)
                    {
                        QRectF boundrect=ptr->calcBoundingRect();
                        drawtree.addNode(boundrect,args[1]+QString(":bounding_rect"));
                    }
                    else if(args.size()==3)
                    {
                        int node_id=exp(args[2]);
                        if(node_id<ptr->nodes.size())
                        {
                            QRectF boundrect=ptr->nodes[node_id].calcBoundingRect();
                            drawtree.addNode(boundrect,args[1]+QString(":node%1:bounding_rect").arg(node_id));
                        }
                        else
                        {
                            return Err(i,args[0]);
                        }
                    }
                }
                else
                {
                    return Err(i,args[0]);
                }
            }
            else if((args.size()==3) && args[0]==QString("ENTITY_REVERSE"))
            {
                Entity * ptr=drawtree.findEntity(args[1]);
                if(ptr)
                {
                    int node_id=exp(args[2]);
                    if(node_id<ptr->nodes.size())
                    {
                        ptr->nodes[node_id].path=ptr->nodes[node_id].path.toReversed();
                    }
                    else{return Err(i,args[0]);}
                }
                else{return Err(i,args[0]);}

            }
            else if((args.size()==8) && args[0]==QString("ENTITY_BOOL"))
            {
                Entity * ptrA=drawtree.findEntity(args[1]);
                Entity * ptrB=drawtree.findEntity(args[3]);

                bool deleteflagA=(args[6]==QString("Delete"));
                bool deleteflagB=(args[7]==QString("Delete"));

                if(ptrA && ptrB)
                {
                    int node_idA=exp(args[2]),node_idB=exp(args[4]);
                    std::cout<<node_idA<<" "<<node_idB<<std::endl;
                    if(node_idA<ptrA->nodes.size() && node_idB<ptrB->nodes.size())
                    {
                        QPainterPath path_bool;
                        if(args[5]==QString("Union"))
                        {
                            path_bool=ptrA->nodes[node_idA].getTransformedPath().united(ptrB->nodes[node_idB].getTransformedPath());
                        }
                        if(args[5]==QString("Intersection"))
                        {
                            path_bool=ptrA->nodes[node_idA].getTransformedPath().intersected(ptrB->nodes[node_idB].getTransformedPath());
                        }
                        if(args[5]==QString("Substract"))
                        {
                            path_bool=ptrA->nodes[node_idA].getTransformedPath().subtracted(ptrB->nodes[node_idB].getTransformedPath());
                        }
                        if(args[5]==QString("SubstractI"))
                        {
                            path_bool=ptrA->nodes[node_idA].getTransformedPath().subtractedInverted(ptrB->nodes[node_idB].getTransformedPath());
                        }
                        drawtree.addNode(path_bool,QString("BOOL:%1").arg(args[5]));

                        if(deleteflagA)ptrA->nodes.removeAt(node_idA);
                        if(deleteflagB)ptrB->nodes.removeAt(node_idB);
                    }
                    else
                    {
                        return Err(i,args[0]);
                    }

                }
                else
                {
                    return Err(i,args[0]);
                }
            }
            else if((args.size()==3) && args[0]==QString("ENTITY_MERGE"))
            {
                Entity * ptr=drawtree.findEntity(args[1]);

                bool deleteflag=(args[2]==QString("Delete"));

                if(ptr && ptr->nodes.size()>0)
                {
                    if(deleteflag)
                    {
                        QPainterPath path_bool;
                        while(ptr->nodes.size()>0)
                        {
                            path_bool=path_bool.united(ptr->nodes.last().getTransformedPath());
                            ptr->nodes.removeLast();
                        }
                        drawtree.addNode(path_bool,QString("MERGE:%1").arg(args[1]));

                    }
                    else
                    {
                        QPainterPath path_bool=ptr->nodes[0].getTransformedPath();
                        for(int i=1;i<ptr->nodes.size();i++)
                        {
                            path_bool=path_bool.united(ptr->nodes[i].getTransformedPath());
                        }
                        drawtree.addNode(path_bool,QString("MERGE:%1").arg(args[1]));
                    }
                }
                else
                {
                    return Err(i,args[0]);
                }
            }
            else if(args.size()==5 && args[0]==QString("DRAW_RECT"))//Ok
            {
                QRectF rect(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4]));
                drawtree.addNode(rect,args[0]);
            }
            else if(args.size()==6 && args[0]==QString("DRAW_RECT_ROUND"))//Ok
            {
                QPainterPath path=roundRectPath(QRectF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])),exp(args[5]));
                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==7 && args[0]==QString("DRAW_PUZZLE"))//Ok
            {
                drawtree.addEntity("Puzzle");

                double x=exp(args[1]);
                double y=exp(args[2]);

                int nx=exp(args[3]);
                int ny=exp(args[4]);
                double sz=exp(args[5]);
                int mode=exp(args[6]);

                double lx=nx*sz;
                double ly=ny*sz;

                for(int i=1;i<nx;i++)
                {
                    for(int j=0;j<ny;j++)
                    {
                        drawtree.addNode(getPuzzleShape(QLine(x+i*sz,y+j*sz,x+i*sz,y+(j+1)*sz),rand()%2+2*mode),"lines");
                    }
                }
                for(int i=1;i<ny;i++)
                {
                    for(int j=0;j<nx;j++)
                    {
                        drawtree.addNode(getPuzzleShape(QLine(x+j*sz,y+i*sz,x+(j+1)*sz,y+i*sz),rand()%2+2*mode),"lines");
                    }
                }

                drawtree.addNode(QRectF(x,y,lx,ly),"contour");

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
                help(args[1]);
            }

            else if(args.size()==2 && args[0]==QString("GET_EV_DC"))//Ok
            {
                double ev=0.0;
                double dc=exp(args[1])*1e-3;

                double tab_dc[37]=
                {
                    0.01,
                    0.012,
                    0.014,
                    0.016,
                    0.018,
                    0.019,
                    0.02,
                    0.021,
                    0.022,
                    0.023,
                    0.024,
                    0.025,
                    0.027,
                    0.028,
                    0.03,
                    0.032,
                    0.034,
                    0.036,
                    0.038,
                    0.04,
                    0.043,
                    0.045,
                    0.048,
                    0.05,
                    0.053,
                    0.056,
                    0.06,
                    0.063,
                    0.067,
                    0.07,
                    0.071,
                    0.075,
                    0.08,
                    0.085,
                    0.09,
                    0.095,
                    0.1
                };

                double tab_ev[37]=
                {
                    0.00135,
                    0.001525,
                    0.0019,
                    0.00215,
                    0.0022,
                    0.002225,
                    0.0025,
                    0.003,
                    0.003,
                    0.003,
                    0.003,
                    0.0035,
                    0.00375,
                    0.004,
                    0.0045,
                    0.0045,
                    0.005,
                    0.00525,
                    0.00525,
                    0.0055,
                    0.00575,
                    0.00625,
                    0.00675,
                    0.0065,
                    0.00675,
                    0.007,
                    0.0075,
                    0.00825,
                    0.00875,
                    0.00875,
                    0.00875,
                    0.00925,
                    0.00925,
                    0.01,
                    0.01,
                    0.01025,
                    0.0105
                };

                double dmin=1e100;
                int nearest_dc_index=0;
                for(int i=0;i<37;i++)
                {
                    double d=std::abs(dc-tab_dc[i]);
                    if(d<dmin)
                    {
                        nearest_dc_index=i;
                        dmin=d;
                    }
                }

                dc=tab_dc[nearest_dc_index]*1e3;
                ev=tab_ev[nearest_dc_index]*1e3;

                sub_process((QString("DEFINE Ev %1;DEFINE Dc %2;").arg(ev).arg(dc)).split(";",QString::SkipEmptyParts),generator,painter,drawtree);
            }

            else if(args.size()==7 && args[0]==QString("DRAW_FLEX"))//Ok
            {
                drawtree.addEntity("Flex");

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

                drawtree.addNode(lines,"lines");
            }
            else if((args.size()==9 || args.size()==10) && args[0]==QString("DRAW_LINE_CRENEAUX"))//Ok
            {
                QPainterPath path;
                double x1=exp(args[1]);
                double y1=exp(args[2]);
                double x2=exp(args[3]);
                double y2=exp(args[4]);


                path.moveTo(QPointF(x1,y1));

                if(args.size()==9)
                {
                    draw_lineCreneaux(path,
                                      QLineF(QPointF(x1,y1),QPointF(x2,y2)),
                                      exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]));
                }
                else if(args.size()==10)
                {
                    draw_lineCreneaux(path,
                                      QLineF(QPointF(x1,y1),QPointF(x2,y2)),
                                      exp(args[5]),exp(args[6]),exp(args[7]),exp(args[8]),exp(args[9]));
                }

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==7 && args[0]==QString("DRAW_CLIP"))
            {
                QPainterPath path;

                double x1=exp(args[1]);
                double y1=exp(args[2]);
                double x2=exp(args[3]);
                double y2=exp(args[4]);
                QLineF line(QPointF(x1,y1),QPointF(x2,y2));
                double E=exp(args[5]);
                double dL=line.length();
                int mode=exp(args[6]);

                QPointF u(line.dx()/dL,line.dy()/dL);
                QPointF v(u.y(),-u.x());

                QPointF cp(x1,y1);
                if(mode==0)
                {
                    path.moveTo(cp);
                    clip(path,cp,u,v,E,dL);
                }
                else if(mode==1)
                {
                    path.moveTo(cp);
                    clipb(path,cp,u,v,E,dL);
                }
                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==15 && args[0]==QString("DRAW_RECT_CRENEAUX"))//Ok
            {
                QPainterPath path;


                int modes[4]={exp(args[11]),exp(args[12]),exp(args[13]),exp(args[14])};

                double W=exp(args[5]);
                double dL=exp(args[6]);

                double x=exp(args[1]);
                double y=exp(args[2]);
                double w=exp(args[3]);
                double h=exp(args[4]);

                if(modes[0]==1 || modes[0]==3 || modes[0]==5 || modes[0]==7){y+=W;h-=W;}
                if(modes[1]==1 || modes[1]==3 || modes[1]==5 || modes[1]==7){w-=W;}
                if(modes[2]==1 || modes[2]==3 || modes[2]==5 || modes[2]==7){h-=W;}
                if(modes[3]==1 || modes[3]==3 || modes[3]==5 || modes[3]==7){x+=W;w-=W;}


                QLineF lineS(x  ,y  ,x+w,y  );//Sud
                QLineF lineO(x  ,y+h,x  ,y  );//Ouest
                QLineF lineN(x+w,y+h,x  ,y+h);//Nord
                QLineF lineE(x+w,y  ,x+w,y+h);//Est

                path.moveTo(QPointF(x,y));
                draw_lineCreneaux(path,lineS,W,dL,exp(args[7]),modes[0]);
                draw_lineCreneaux(path,lineE,W,dL,exp(args[8]),modes[1]);
                draw_lineCreneaux(path,lineN,W,dL,exp(args[9]),modes[2]);
                draw_lineCreneaux(path,lineO,W,dL,exp(args[10]),modes[3]);
                drawtree.addNode(path,args[0]);
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
                drawtree.addNode(path,args[0]);
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

            else if(args.size()==2 && args[0]==QString("SUB_SCRIPT"))//Ok
            {
                bool ok;
                QStringList subcontent=loadScript(args[1],ok);
                if(ok)
                {
                    Err err=this->sub_process(subcontent,generator,painter,drawtree);
                    if(err.cmd!=QString("None"))
                    {
                        return err;
                    }
                }
                else
                {
                    te_console->append(QString("Impossible d'ouvrir : %1").arg(args[1]));
                    return Err(i,args[0]);
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
                drawtree.addNode(vlines,args[0]);
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

                drawtree.addNode(makePolygon(getIFS(seed,pattern,exp(args[4]))),args[0]);
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
                drawtree.addNode(vlines,args[0]);
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
                drawtree.addNode(vlines,args[0]);
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
                drawtree.addNode(vlines,args[0]);
            }
            else if (args.size()==4 && args[0]==QString("TRANSFORM_ROTATE"))
            {
                QTransform t;
                t.translate(exp(args[1]),exp(args[2]));
                t.rotate(exp(args[3]));
                t.translate(-exp(args[1]),-exp(args[2]));
                transform*=t;
                drawtree.setTransform(transform);
            }
            else if (args.size()==3 && args[0]==QString("TRANSFORM_TRANSLATE"))
            {
                QTransform t;
                t.translate(exp(args[1]),exp(args[2]));
                transform*=t;
                drawtree.setTransform(transform);
            }
            else if (args.size()==5 && args[0]==QString("TRANSFORM_SCALE"))
            {
                QTransform t;
                t.translate(exp(args[1]),exp(args[2]));
                t.scale(exp(args[3]),exp(args[4]));
                t.translate(-exp(args[1]),-exp(args[2]));
                transform*=t;
                drawtree.setTransform(transform);
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
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),args[3].replace("$"," "));
                }
                else if(args.size()==5)
                {
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),args[3].replace("$"," ").arg(exp(args[4])));
                }
                else if(args.size()==6)
                {
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),args[3].replace("$"," ").arg(exp(args[4])).arg(exp(args[5])));
                }
                drawtree.addNode(path,args[0]+QString(":")+args[3]);
            }
            else if (args.size()==4 && args[0]==QString("TEXTC"))
            {
                QPainterPath path;

                unsigned int id=((int)exp(args[3]))%26;

                QString tab[26]={"A","B","C","D","E","F","G","H","I",
                               "J","K","L","M","N","O","P","Q","R",
                               "S","T","U","V","W","X","Y","Z"};

                if(args.size()==4)
                {
                    path.addText(QPointF(exp(args[1]),exp(args[2])),painter.font(),tab[id]);
                }

                drawtree.addNode(path,args[0]+QString(":")+args[3]);
            }
            else if (args.size()==19 && args[0]==QString("PROJECT_OBJECT_MAP"))
            {
                bool ok=calcGnomonicProjection(painter,args[1],args[2],exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]),exp(args[7]),
                        Vector3d(exp(args[8]),exp(args[9]),exp(args[10])),exp(args[11]),exp(args[12]),exp(args[13])
                        ,exp(args[14]),exp(args[15]),exp(args[16]),exp(args[17]),exp(args[18]));
                if(!ok)return Err(i,args[0]);
            }
            else if (args.size()==9 && args[0]==QString("PROJECT_OBJECT"))
            {
                QPainterPath path;
                bool ok=calcProjection(path,
                                       args[1],
                        QPointF(exp(args[2]),exp(args[3])),
                        Vector3d(exp(args[4]),exp(args[5]),exp(args[6])),
                        exp(args[7]),exp(args[8]));

                if(!ok)return Err(i,args[0]);

                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==8 && args[0]==QString("DRAW_GEAR"))
            {
                QPainterPath path;
                draw_gear( path,exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]),exp(args[7]));

                drawtree.getTransform().translate(exp(args[1]),exp(args[2]));
                drawtree.addNode(path,args[0]);
                drawtree.getTransform().translate(-exp(args[1]),-exp(args[2]));
            }
            else if(args.size()==7 && args[0]==QString("DRAW_GEAR_R"))
            {
                QPainterPath path;
                draw_gear_r( path,exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]));

                drawtree.getTransform().translate(exp(args[1]),exp(args[2]));
                drawtree.addNode(path,args[0]);
                drawtree.getTransform().translate(-exp(args[1]),-exp(args[2]));
            }
            else if(args.size()==7 && args[0]==QString("DRAW_GEAR_S"))
            {
                QPainterPath path;
                draw_gear_s( path,exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]));

                drawtree.getTransform().translate(exp(args[1]),exp(args[2]));
                drawtree.addNode(path,args[0]);
                drawtree.getTransform().translate(-exp(args[1]),-exp(args[2]));
            }
            else if(args.size()==7 && args[0]==QString("DRAW_PENDULE"))
            {
                QPainterPath path;
                draw_pendule(path,exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4]),exp(args[5]),exp(args[6]));
                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==6 && args[0]==QString("DRAW_CIRCLE_TANGENT"))
            {
                QPainterPath path=draw_circle_tangent(QLineF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])),exp(args[5]));
                drawtree.addNode(path,args[0]);
            }
            else if(args.size()==10 && args[0]==QString("PLOT"))
            {
                QPainterPath path;
                bool ok=plot(painter,path,QRectF(exp(args[1]),exp(args[2]),exp(args[3]),exp(args[4])),
                        exp(args[5]),exp(args[6]),
                        args[7],
                        exp(args[8]),exp(args[9]));

                if(!ok)return Err(i,args[0]);

                drawtree.addNode(path,args[0]);
            }
            else
            {
                return Err(i,args[0]);
            }
        }
    }

    return Err(0,"None");
}

Err MainWindow::process(QStringList content)
{
    transform.reset();
    te_console->clear();
    QSvgGenerator generator;
    //EpsPaintDevice generator;
    QPainter painter;
    DrawTree drawtree;

    return sub_process(content,generator,painter,drawtree);
}

std::vector<std::vector<double>> MainWindow::loadCSV(QString filename)
{
    std::vector<std::vector<double>> data;

    QFile file(filename);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        int id=0;
        while(!file.atEnd())
        {
            QStringList line=QString(file.readLine()).split(";");

            if(id==0)
            {
                data.resize(line.size());
            }
            else if(data.size()==line.size())
            {
                for(int i=0;i<data.size();i++)
                {
                    data[i].push_back(line[i].toDouble());
                }
            }
            else
            {
                std::cout<<"Load CSV error line :"<<id<<std::endl;
            }

            id++;
        }

        file.close();
    }

    return data;
}

QPointF getScaled(const QPointF & p,const QRectF & area,const  QRectF & scale)
{
    return QPointF (
                (p.x()-scale.x())/scale.width ()*area.width ()+area.x(),
                (p.y()-scale.y())/scale.height()*area.height()+area.y());
}

bool MainWindow::plot(QPainter & painter,QPainterPath & path,QRectF area, double scaleX, double scaleY, QString filename, int idX,int idY)
{
    //Load Data
    std::vector<std::vector<double>> data=loadCSV(filename);
    if(data.size()>0)
    {
        te_console->append(QString("PLOT : Load CSV size=(%1 x %2)").arg(data[0].size()).arg(data.size()));
    }
    else
    {
        te_console->append(QString("PLOT : Load CSV failed"));
        return false;
    }

    if(idX>=data.size() && idY>=data.size()){te_console->append(QString("PLOT : Out of range"));return false;}

    //Plot data
    QRectF scale;
    double minX=*std::min_element(data[idX].begin(),data[idX].end());
    double minY=*std::min_element(data[idY].begin(),data[idY].end());
    double deltaX=*std::max_element(data[idX].begin(),data[idX].end())-minX;
    double deltaY=*std::max_element(data[idY].begin(),data[idY].end())-minY;
    double meanX=minX+deltaX*0.5,meanY=minY+deltaY*0.5;

    scale.setX( meanX -deltaX/2 * scaleX);
    scale.setY( meanY -deltaY/2 * scaleY );
    scale.setWidth ( deltaX * scaleX);
    scale.setHeight( deltaY * scaleY);

    for(int i=0;i<data[idX].size();i++)
    {
        QPointF p(data[idX][i],data[idY][i]);
        QPointF ps=getScaled(p,area,scale);

        if(i==0){path.moveTo(ps);}
        else{path.lineTo(ps);}
    }

    path.addText(getScaled( QPointF(minX+deltaX,minY)  ,area,scale) ,painter.font(), QString(".%1").arg(minY) );
    path.addText(getScaled( QPointF(minX+deltaX,meanY)  ,area,scale) ,painter.font(), QString("dY=%1").arg(deltaY) );
    path.addText(getScaled( QPointF(minX+deltaX,minY+deltaY)  ,area,scale) ,painter.font(), QString(".%1").arg(minY+deltaY) );

    return true;
}

void MainWindow::search()
{
    highlighter_script->clearSubRules();

    QStringList searchString = te_console->toPlainText().split("\n",QString::SkipEmptyParts);

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(Qt::red);
    keywordFormat.setFontWeight(QFont::Bold);

    for(int k=0;k<searchString.size();k++)
    {
        highlighter_script->addSubRule(QString("\\b%1\\b").arg(searchString[k]),keywordFormat);
    }

    highlighter_script->rehighlight();
}

QPainterPath convertImage(QImage image,double threshold)
{

}
