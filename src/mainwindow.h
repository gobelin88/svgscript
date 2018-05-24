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
#include <QScrollArea>
#include <obj.h>

#include <displayer.h>

#include <Eigen/Dense>

#include "Highlighter.h"

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

public slots:
    void slot_load();
    void slot_save();
    void slot_run();
    void slot_direct_save();
    void slot_direct_load(QString filename);
    void slot_modified();

private:
    Ui::MainWindow *ui;

    QHBoxLayout * l_pb;

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

    void draw_ellipseCreneaux(QPainterPath & pts, const QPointF & center,double ra,double rb, double E, double dL, int n, int mode, double offset=0);
    void draw_lineCreneaux(QPainterPath & pts, const QLineF & line, double E, double dL, int n, int mode, double offset=0);
    void draw_Line(QPainter & painter,const QPointF & pa,const QPointF & pb);

    QTransform transform;//Global transform

    Displayer * displayer;

    QString version;
};

#endif // MAINWINDOW_H
