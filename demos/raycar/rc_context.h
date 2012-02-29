
#if !defined(rc_context_h)
#define rc_context_h

class GLContext
{
public:
    static GLContext *context();
};

void glAssertError_(char const *file, int line);
#define glAssertError() glAssertError_(__FILE__, __LINE__)



#endif  //  rc_context_h
