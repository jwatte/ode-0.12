
#if defined(_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define GLUT_DISABLE_ATEXIT_HACK 1
#define WINDOWS_MAIN 1
#define UNIX_MAIN 0
#else
#define WINDOWS_MAIN 0
#define UNIX_MAIN 1
#endif

#include <GL/glew.h>
#include <glut.h>
#include <vector>
#include <algorithm>


#if WINDOWS_MAIN

int CALLBACK WinMain(
  __in  HINSTANCE hInstance,
  __in  HINSTANCE hPrevInstance,
  __in  LPSTR lpCmdLine,
  __in  int nCmdShow
)
{
    //  translate lpCmdLine to argv-style array, allowing for 
    //  double-quotes to quote arguments with spaces
    std::vector<std::string> argvstore;
    std::string curOut;
    char const *str = lpCmdLine;
    char const *start = str;
    bool quoted = false;
    while (*str)
    {
        if (*str == ' ' && !quoted)
        {
            if (str > start)
            {
                argvstore.push_back(curOut);
                curOut = "";
            }
            start = str + 1;
        }
        else if (*str == '"') 
        {
            if (str > start && str[-1] == '"')
            {
                // "" means one double-quote
                curOut.push_back(*str);
            }
            quoted = !quoted;
        }
        else
        {
            curOut.push_back(*str);
        }
        ++str;
    }
    if (curOut.size() > 0)
    {
        argvstore.push_back(curOut);
        curOut = "";
    }
    std::vector<char *> argv;
    for (std::vector<std::string>::iterator ptr(argvstore.begin()), end(argvstore.end());
        ptr != end; ++ptr)
    {
        argv.push_back(const_cast<char *>((*ptr).c_str()));
    }
    argv.push_back(0);

    //  init GLUT
    int argc = argv.size() - 1;
    char **argvp = &argv[0];
    glutInitWindowSize(1024, 576);
    glutInit(&argc, argvp);

    //  init GLEW

    //  run
    glutMainLoop();

    return 0;
}
#endif

#if UNIX_MAIN

#endif
