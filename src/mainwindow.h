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
#include "codeeditor.h"

class MyQSlider;

struct Err
{
    Err(int id,QString cmd)
    {
        this->id=id;
        this->cmd=cmd;
    }

    int id;
    QString cmd;
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

    bool calcProjection(QPainterPath & path,QString obj_filename,QPointF c,Vector3d euler_angles,float scale,int mode);


    bool calcGnomonicProjection(QPainter & painter,
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
    void removeSliders();
    void initSliders();

    void search();

private:
    std::vector<std::vector<double>> loadCSV(QString filename);
    bool plot(QPainter &painter, QPainterPath &path, QRectF area, double scaleX, double scaleY, QString filename, int idX, int idY);

    Ui::MainWindow *ui;

    QHBoxLayout * l_pb;
    QVBoxLayout * slider_layout;

    QScrollArea * scroll;

    QPushButton * pb_load;
    QPushButton * pb_save;
    QPushButton * pb_run;
    QPushButton * pb_search;
    QAction * a_direct_save;

    CodeEditor * te_script;
    QTextEdit * te_console;
    QSvgWidget * w_svg;

    Err process(QStringList content);

    QString current_file_path;
    QString current_svg_path;

    double exp(QString expression);

    QScriptEngine * pe;

    Highlighter * highlighter;

    QString comment(QString content);

    void draw_gear_s(QPainterPath & path, double m, int n, double daxe, int nb_spokes=-1);
    void draw_gear_r(QPainterPath & path, double m, int n, double alpha, double daxe, int nb_spokes=-1);
    void draw_gear(QPainterPath & path, double m, int n, double alpha, double daxe, int nb_spokes=-1);
    void draw_ellipseCreneaux(QPainterPath & pts, const QPointF & center, double ra, double rb, double E, double dL, int n, int mode, double offset=0, double theta0=0, double dtheta=360);
    void draw_lineCreneaux(QPainterPath & pts, const QLineF & line, double E, double dL, int n, int mode, double offset=0);
    void draw_Line(QPainter & painter,const QPointF & pa,const QPointF & pb);
    void draw_pendule(QPainter & painter,double x,double y,double P,double theta,double daxe1,double daxe2);
    QPainterPath draw_circle_tangent(QLineF line,double alpha);

    void calc_bobine(QPainter & painter, double Di, double N, double W, double S, double nbLayers, double t, QString type);

    void help(QString cmd);

    QTransform transform;//Global transform

    Displayer * displayer;

    QString version;

    QList<MyQSlider*> sliderlist;

    QTabWidget * tab_view;


};

class MyQSlider:public QWidget
{
    Q_OBJECT
public:
    MyQSlider(QString varname,double min,double max,double value_init,MainWindow * gui)
    {
        slider=new QSlider(Qt::Horizontal,gui);
        spin=new QDoubleSpinBox(gui);
        spin->setFixedWidth(100);
        spin->setPrefix(varname+QString("="));
        spin->setRange(min,max);
        pb_close=new QPushButton("X",this);
        pb_init=new QPushButton("Init",this);


        //this->setMaximumWidth(400);
        this->gui=gui;
        slider->setRange(0,1000);
        this->min=min;
        this->max=max;
        this->value_init=value_init;
        this->varname=varname;
        this->setAttribute( Qt::WA_DeleteOnClose );
        this->set_value(value_init);

        QHBoxLayout * hlayout=new QHBoxLayout(this);
        hlayout->addWidget(pb_init);
        hlayout->addWidget(slider);
        hlayout->addWidget(spin);
        hlayout->addWidget(pb_close);        

        connect(pb_close,SIGNAL(clicked(bool)),this,SLOT(close()));
        connect(slider,SIGNAL(valueChanged(int)),this,SLOT(updateValue_slider(int)));
        connect(pb_init,SIGNAL(clicked(bool)),this,SLOT(init()));
        connect(spin,SIGNAL(valueChanged(double)),this,SLOT(updateValue_spin(double)));
    }
    ~MyQSlider()
    {
        emit deleted(varname);
    }

    QString name(){return varname;}

    double get_value(){return valuef;}

public slots:
    void reset()
    {
        set_value(value_init);
    }

    void init()
    {
        set_value(value_init);
        gui->slot_run();
    }

    void updateValue_slider(int valuei)
    {
        valuef=valuei*(max-min)/1000.0+min;

        spin->blockSignals(true);
        spin->setValue(valuef);
        spin->blockSignals(false);

        gui->slot_run();
    }

    void updateValue_spin(double valuef)
    {
        int valuei=(valuef-min)*1000.0/(max-min);
        this->valuef=valuef;

        slider->blockSignals(true);
        slider->setValue(valuei);
        slider->blockSignals(false);

        gui->slot_run();
    }

    void set_value(double valuef)
    {
        int valuei=(valuef-min)*1000.0/(max-min);
        this->valuef=valuef;

        spin->blockSignals(true);
        spin->setValue(valuef);
        spin->blockSignals(false);

        slider->blockSignals(true);
        slider->setValue(valuei);
        slider->blockSignals(false);
    }

    QSlider * obj(){return slider;}

signals:
    void deleted(QString varname);

private:


    QString varname;
    double min,max,valuef,value_init;
    MainWindow * gui;

    QSlider * slider;
    QDoubleSpinBox * spin;
    QPushButton * pb_close,*pb_init;
};

#endif // MAINWINDOW_H
