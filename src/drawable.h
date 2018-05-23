#ifndef DRAWABLE_H
#define DRAWABLE_H

#include <QOpenGLFunctions_1_0>

class Drawable
{
public:
    virtual void draw(QOpenGLFunctions_1_0 *f)=0;
};

#endif // DRAWABLE_H
