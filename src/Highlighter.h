#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

struct Command
{
    Command(QString keyword,QString help){this->keyword=keyword;this->help=help;}
    QString keyword;
    QString help;
};

#define NB_CMD 20
static const Command CommandsList[NB_CMD]={
    Command("HELP","Lol"),

    Command("SVG_BEGIN","W H :\n\nCrée un SVG de dimension (W,H). Une instruction SVG_END devra terminer le script."),
    Command("SVG_END"," :\n\nTermine le script et enregistre le SVG."),
    Command("SVG_PEN","r v b t:\n\nDéfinit la couleur (r,v,b) et le type (t) du crayon. (t) peut prendre les valeurs (None,Solid,Dash,Dot)."),
    Command("SVG_BRUSH","r v b t:\n\nDéfinit la couleur (r,v,b) et le type (t) du pinceau. (t) peut prendre les valeurs (None,Solid)."),

    Command("DEFINE","var value :\n\nDéfinition ou modification d'une variable (var) avec la valeur (val)."),
    Command("DISP","var:\n\nAffiche la valeur d'une variable ou d'une expression. voir DEFINE"),
    Command("LABEL","var:\n\nDéfinit une étiquette (var). voir GOTO."),
    Command("GOTO","var val:\n\nSe rend à l'étiquette (var) tant que (var) n'est pas égale à (val). voir LABEL."),

    Command("DRAW_FLEX","x y w h a b\n\nDessine une zone flexible rectangulaire dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h). Puis dessine des traits de manière de longueur (b) et espacé de (a)"),
    Command("DRAW_PATH"," T ... \n\nDessine un chemin continu. la commande T peut prendre les valeurs (M,L,C,E) chacune doit être suivie des commandes appropriés \n\nM x y: Move to (x,y)\nL x y: Line to (x,y)\nC x y r a b:Circle to (x,y) de rayon (r) d'angle de depart (a) et d'angle (b)\nE x y ra rb a b:Ellipse to (x,y) de rayons (ra,rb) d'angle de depart (a) et d'angle (b).."),
    Command("DRAW_PATH_R"," T ... \n\nDessine un chemin continu relatif. la commande T peut prendre les valeurs (M,L,C,E) chacune doit être suivie des commandes appropriés \n\nM x y: Move to (x,y)\nL x y: Line to (x,y)\nC x y r a b:Circle to (x,y) de rayon (r) d'angle de depart (a) et d'angle (b)\nE x y ra rb a b:Ellipse to (x,y) de rayons (ra,rb) d'angle de depart (a) et d'angle (b).."),
    Command("DRAW_CIRCLE","x y r :\n\nDessine un cercle de rayon (r) en (x,y)."),
    Command("DRAW_ELLIPSE","x y r1 r2 :\n\nDessine une ellipse de rayons (r1) et (r2) en (x,y)."),
    Command("DRAW_LINE","x1 y1 x2 y2 :\n\nDessine une ligne du point (x1,y2) au point (x2,y2)."),
    Command("DRAW_LINE_CRENEAUX","x1 y1 x2 y2 w dL n t (d):\n\nDessine une ligne en créneau de longueur (dL) et d'epaisseur (w) du point (x1,y2) au point (x2,y2).\nLe parametre (n) nombres de creneaux -1 pour le calcul automatique et (t) le type de creneau."),
    Command("DRAW_RECT","x y w h:\n\nDessine un rectangle dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h)."),
    Command("DRAW_RECT_ROUND","x y w h r:\n\nDessine un rectangle au coins arrondi de rayon (r) dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h)."),
    Command("DRAW_RECT_CRENEAUX","x y w h w dL n1 n2 n3 n4 t1 t2 t3 t4 (d1) (d2) (d3) (d4) :\n\nDessine un rectangle en creneaux dont le coin supérieur gauche est en (x,y) de largeur (w) et de hauteur (h) les creneaux sont définit dans le sens horaires et on les mêmes parametres que DRAW_LINE_CRENEAU." ),
    Command("PROJECT_OBJECT","file.obj ...")
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
