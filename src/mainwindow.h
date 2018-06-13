#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QtSvg/QSvgWidget>
#include <QtSvg/QSvgGenerator>
#include <QtSvg/QSvgRenderer>
#include <QGridLayout>
#include <QAction>
#include <QPainter>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <iostream>
#include <QScriptEngine>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <obj.h>
#include <QSlider>

#include <displayer.h>

#include <Eigen/Dense>

#include "Highlighter.h"

class MyQSlider;

struct Err
{
    Err(int line,QString what)
    {
        this->line=line;
        this->what=what;
    }

    int line;
    QString what;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void loadStyle(QString filename);

    void calcGnomonicProjection(QPainter & painter,
                                QString obj_filename,
                                QString map_filename,
                                int res,
                                float scale,
                                bool meridiens,
                                int Nlat,
                                int Nlon,
                                Vector3d euler_angles,
                                double radius, int idpN, int idpS, double W, double dL, double dA, double marge);

    QScriptEngine *engine(){return pe;}

public slots:
    void slot_load();
    void slot_save();
    void slot_run();
    void slot_direct_save();
    void slot_direct_load(QString filename);
    void slot_modified();
    void removeSlider(QString varname);

private:
    Ui::MainWindow *ui;

    QHBoxLayout * l_pb;
    QVBoxLayout * slider_layout;

    QScrollArea * scroll;

    QPushButton * pb_load;
    QPushButton * pb_save;
    QPushButton * pb_run;
    QAction * a_direct_save;

    QTextEdit * te_script;
    QTextEdit * te_console;
    QSvgWidget * w_svg;

    Err process(QStringList content);

    QString current_file_path;
    QString current_svg_path;

    double exp(QString expression);

    QScriptEngine * pe;

    Highlighter * highlighter;

    QString comment(QString content);

    void draw_gear(QPainterPath & path,double x,double y,double dtooth,int ntooth,double e,double a);
    void draw_ellipseCreneaux(QPainterPath & pts, const QPointF & center,double ra,double rb, double E, double dL, int n, int mode, double offset=0);
    void draw_lineCreneaux(QPainterPath & pts, const QLineF & line, double E, double dL, int n, int mode, double offset=0);
    void draw_Line(QPainter & painter,const QPointF & pa,const QPointF & pb);

    QTransform transform;//Global transform

    Displayer * displayer;

    QString version;

    QList<MyQSlider*> sliderlist;


};

class MyQSlider:public QWidget
{
    Q_OBJECT
public:
    MyQSlider(QString varname,double min,double max,MainWindow * gui)
    {
        slider=new QSlider(Qt::Horizontal,gui);
        spin=new QDoubleSpinBox(gui);
        spin->setFixedWidth(100);
        spin->setPrefix(varname+QString("="));
        spin->setRange(min,max);
        pb_close=new QPushButton("X",this);


        this->setMaximumWidth(400);
        this->gui=gui;
        slider->setRange(0,1000);
        this->min=min;
        this->max=max;
        this->varname=varname;
        this->setAttribute( Qt::WA_DeleteOnClose );

        QHBoxLayout * hlayout=new QHBoxLayout(this);
        hlayout->addWidget(slider);
        hlayout->addWidget(spin);
        hlayout->addWidget(pb_close);

        connect(pb_close,SIGNAL(clicked(bool)),this,SLOT(close()));
    }
    ~MyQSlider()
    {
        emit deleted(varname);
    }

    QString name(){return varname;}

    double get_value(){return valuef;}

    void set_value(double valuef)
    {
        int valuei=(valuef-min)*1000/(max-min);
        this->valuef=valuef;
        spin->setValue(valuef);
        slider->setValue(valuei);
    }



public slots:
    void updateValue(int valuei)
    {
        valuef=valuei*(max-min)/1000+min;
        spin->setValue(valuef);
        gui->slot_run();
    }

    QSlider * obj(){return slider;}

signals:
    void deleted(QString varname);

private:
    QString varname;
    double min,max,valuef;
    MainWindow * gui;

    QSlider * slider;
    QDoubleSpinBox * spin;
    QPushButton * pb_close;
};

#endif // MAINWINDOW_H
