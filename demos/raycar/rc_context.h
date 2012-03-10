
#if !defined(rc_context_h)
#define rc_context_h

#include "rc_math.h"

class GLContext
{
public:
    static GLContext *context();
    void realize(int width, int height);
    Vec3 size();
private:
    int width_;
    int height_;
};

void glAssertError_(char const *file, int line);
#define glAssertError() glAssertError_(__FILE__, __LINE__)



#endif  //  rc_context_h
