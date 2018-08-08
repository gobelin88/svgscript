#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QString>
#include <QTranslator>

struct Command
{
    Command(QString keyword,QString help){this->keyword=keyword;this->help=help;}
    QString keyword;
    QString help;
};

#define NB_CMD 35
static const Command CommandsList[NB_CMD]={
    Command("HELP",              QObject::tr("Lol")),

    Command("SVG_BEGIN",         QObject::tr("W H :\n\nCrée un SVG de dimension (W,H). Une instruction SVG_END devra terminer le script.")),
    Command("SVG_END",           QObject::tr(" :\n\nTermine le script et enregistre le SVG.")),
    Command("SVG_PEN",           QObject::tr("r v b t:\n\nDéfinit la couleur (r,v,b) et le type (t) du crayon. (t) peut prendre les valeurs (None,Solid,Dash,Dot).")),
    Command("SVG_BRUSH",         QObject::tr("r v b t:\n\nDéfinit la couleur (r,v,b) et le type (t) du pinceau. (t) peut prendre les valeurs (None,Solid).")),

    Command("DEFINE",            QObject::tr("var value :\n\nDéfinition ou modification d'une variable (var) avec la valeur (val).")),
    Command("SLIDE",             QObject::tr("var value min max:\n\nDéfinition ou modification d'une variable (var) avec la valeur (val) plus slider (min,max).")),
    Command("DISP",              QObject::tr("var:\n\nAffiche la valeur d'une variable ou d'une expression. voir DEFINE")),
    Command("LABEL",             QObject::tr("var:\n\nDéfinit une étiquette (var). voir GOTO.")),
    Command("GOTO",              QObject::tr("var val:\n\nSe rend à l'étiquette (var) tant que (var) n'est pas égale à (val). voir LABEL.")),

    Command("TRANSFORM_ROTATE",   QObject::tr("x y alpha:\n\n Rotation d'angle (alpha) autour de (x,y) de la transformation courante.")),
    Command("TRANSFORM_TRANSLATE",QObject::tr("dx dy:\n\n Translation de vecteur (dx,dy) de la transformation courante.")),
    Command("TRANSFORM_SCALE",    QObject::tr("sx sy:\n\n Facteur d'echelle sur la transformation courante.")),

    Command("DRAW_FLEX",         QObject::tr("x y w h a b\n\nDessine une zone flexible rectangulaire dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h). Puis dessine des traits de manière de longueur (b) et espacé de (a)")),
    Command("DRAW_PATH",         QObject::tr(" T ... \n\nDessine un chemin continu. la commande T peut prendre les valeurs (M,L,C,E) chacune doit être suivie des commandes appropriés \n\nM x y: Move to (x,y)\nL x y: Line to (x,y)\nC x y r a b:Circle to (x,y) de rayon (r) d'angle de depart (a) et d'angle (b)\nE x y ra rb a b:Ellipse to (x,y) de rayons (ra,rb) d'angle de depart (a) et d'angle (b)..")),
    Command("DRAW_PATH_R",       QObject::tr(" T ... \n\nDessine un chemin continu relatif. la commande T peut prendre les valeurs (M,L,C,E) chacune doit être suivie des commandes appropriés \n\nM x y: Move to (x,y)\nL x y: Line to (x,y)\nC x y r a b:Circle to (x,y) de rayon (r) d'angle de depart (a) et d'angle (b)\nE x y ra rb a b:Ellipse to (x,y) de rayons (ra,rb) d'angle de depart (a) et d'angle (b)..")),
    Command("DRAW_CIRCLE",       QObject::tr("x y r :\n\nDessine un cercle de rayon (r) en (x,y).")),
    Command("DRAW_CIRCLE_CRENEAUX",       QObject::tr("x y r w dL n t:\n\nDessine un cercle de rayon (r) en (x,y).  Créneaux de longueur (dL) et d'epaisseur (w)\nLe parametre (n) nombres de creneaux -1 pour le calcul automatique et (t) le type de creneau")),
    Command("DRAW_ELLIPSE",      QObject::tr("x y r1 r2 :\n\nDessine une ellipse de rayons (r1) et (r2) en (x,y).")),
    Command("DRAW_ELLIPSE_CRENEAUX",      QObject::tr("x y r1 r2 w dL n t:\n\nDessine une ellipse de rayons (r1) et (r2) en (x,y). Créneaux de longueur (dL) et d'epaisseur (w)\nLe parametre (n) nombres de creneaux -1 pour le calcul automatique et (t) le type de creneau")),

    Command("DRAW_LINE",         QObject::tr("x1 y1 x2 y2 :\n\nDessine une ligne du point (x1,y2) au point (x2,y2).")),
    Command("DRAW_LINE_CRENEAUX",QObject::tr("x1 y1 x2 y2 w dL n t (d):\n\nDessine une ligne en créneau de longueur (dL) et d'epaisseur (w) du point (x1,y2) au point (x2,y2).\nLe parametre (n) nombres de creneaux -1 pour le calcul automatique et (t) le type de creneau.")),
    Command("DRAW_LINE_ROSACE",  QObject::tr("x1 y1 x2 y2 u r:\n\nDessine un élement de rosace partant de (x1,y1) et finissant à (x2,y2). (u) est un offset sur le départ des point selon le vecteur directeur, et (r) le rayon des arc. Si (r<n/2) alors rien ne se déssinera.")),
    Command("DRAW_RECT",         QObject::tr("x y w h:\n\nDessine un rectangle dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h).")),
    Command("DRAW_RECT_ROUND",   QObject::tr("x y w h r:\n\nDessine un rectangle au coins arrondi de rayon (r) dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h).")),
    Command("DRAW_RECT_CRENEAUX",QObject::tr("x y w h w dL n1 n2 n3 n4 t1 t2 t3 t4 (d1) (d2) (d3) (d4) :\n\nDessine un rectangle en creneaux dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h) les creneaux sont définit dans le sens horaires et on les mêmes parametres que DRAW_LINE_CRENEAU." )),
    Command("DRAW_POLYGON",      QObject::tr("x y r n angle:\n\n Dessine un polygone en (x,y) de (n) cotés tourné de (angle).")),
    Command("DRAW_PUZZLE",       QObject::tr("x y nx ny size:\n\n Dessine un puzzle aléatoire en (x,y) de (nx) x (ny) pieces de tailles (size)")),

    Command("DRAW_GEAR",QObject::tr("x y m n alpha daxe nb_spokes :\n\n Dessine un pignon")),
    Command("DRAW_GEAR_R",QObject::tr("x y m n alpha daxe nb_spokes :\n\n Dessine un pignon type reverse")),
    Command("DRAW_GEAR_S",QObject::tr("x y m n daxe nb_spokes :\n\n Dessine un pignon type spikes")),

    Command("TEXT_FONT",QObject::tr("police taille :\n\n (police)=Arial Helvetica Times Roman")),
    Command("TEXT",QObject::tr("x y text (arg1) (arg2) :\n\n Dessine un text avec de possible arguments arg1 et arg2 aux endroits spécifiés pas %1 et %2")),

    Command("DRAW_PENDULE",QObject::tr("x y P Theta0 daxe1 daxe2: (P)Periode (daxe1-2)Diamètres des trous (Theta0)Angle du lancer pour la correction de la période... \n\n")),

    Command("PROJECT_OBJECT",    QObject::tr("file.obj ..."))
};

class QTextDocument;

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat subkeywordFormat;
    QTextCharFormat variablesFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat chiffresFormat;
};

#endif // HIGHLIGHTER_H
