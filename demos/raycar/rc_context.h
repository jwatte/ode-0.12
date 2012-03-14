
#if !defined(rc_context_h)
#define rc_context_h

#include "rc_math.h"

struct Material;

class BuiltMaterial
{
public:
    virtual void release() = 0;
    virtual void apply() = 0;
};

class GLContext
{
public:
    static GLContext *context();
    void realize(int width, int height);
    Vec3 size();
    BuiltMaterial *buildMaterial(Material const &mtl);

    void preClear();
    void preRender();
    void preSwap();

    void beginCustom(Matrix const &modelView);
    void endCustom();
private:
    int width_;
    int height_;
};

void glAssertError_(char const *file, int line);
#define glAssertError() glAssertError_(__FILE__, __LINE__)



#endif  //  rc_context_h
