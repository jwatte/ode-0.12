
#if defined(_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define GLUT_DISABLE_ATEXIT_HACK 1
#define WINDOWS_MAIN 1
#define UNIX_MAIN 0
HINSTANCE hInstance;
#else
#define WINDOWS_MAIN 0
#define UNIX_MAIN 1
#endif

#include <GL/glew.h>
#include <vector>
#include <algorithm>
#include <string>


#include "rc_window.h"
#include "rc_scene.h"
#include "rc_model.h"
#include "rc_ixfile.h"
#include "rc_asset.h"
#include "rc_context.h"
#include "rc_scenegraph.h"
#include "rc_reference.h"
#include "rc_bitmap.h"

bool quitAfterArgs = false;
bool buildDeps = false;
bool noDeps = false;

int width = 1024;
int height = 576;
double maxStepTime = 0.025;
double stepTime = 1.0 / 120.0;

//  used when converting
float gScale = 1.0f;

void obj_to_bin(char const *from, char const *to, float scale)
{
    IxRead *rd = IxRead::readFromFile(from);
    IxWrite *wr = IxWrite::writeToFile(to);
    ModelWriter *mw = ModelWriter::writeToFile(wr);

    read_obj(rd, mw, scale, dirname(from));

    delete mw;
    delete rd;
}

void build_asset(std::string const &src)
{
    std::string dst(Reference::makeOutputName(src));
    std::string ext = fileext(src);
    if (ext == "obj")
    {
        fprintf(stderr, "Converting obj %s to bin %s with scale %g\n", src.c_str(), dst.c_str(), gScale);
        obj_to_bin(src.c_str(), dst.c_str(), gScale);
    }
    else if (ext == "tga" || ext == "jpg" || ext == "png")
    {
        fprintf(stderr, "Converting image %s to %s\n", src.c_str(), dst.c_str());
        image_build(src, dst);
    }
    else if (ext == "scn")
    {
        fprintf(stderr, "Converting scn %s to %s\n", src.c_str(), dst.c_str());
        //  todo: build scene file by loading it, extracting references, and then saving it
        copy_file(src, dst);
    }
    else
    {
        throw std::runtime_error(std::string("Don't know how to build asset: ") + src);
    }
    std::string str(dst + "::\t" + src + "\nall::\t" + dst + "\n");
    std::string dst_d(dst + ".d");
    IxWrite *iw = IxWrite::writeToFile(dst_d.c_str());
    iw->write(str.c_str(), str.size());
    delete iw;
}

char const *requireArg(char const *argv[], int &argc)
{
    if (argv[argc+1] == 0)
    {
        error(std::string("Option requires an argument: ") + argv[argc]);
    }
    ++argc;
    return argv[argc];
}

void parseArgs(int argc, char const *argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "--size"))
            {
                sscanf_s(requireArg(argv, i), " %d x %d", &width, &height);
            }
            else if (!strcmp(argv[i], "--scale"))
            {
                char const *scl = requireArg(argv, i);
                float a = 0;
                float b = 0;
                int n = sscanf_s(scl, "%f / %f", &a, &b);
                if (n == 1)
                {
                    b = 1;
                }
                if (n == 0 || b <= 0)
                {
                    throw std::runtime_error(std::string("bad --scale argument: ") + scl);
                }
                gScale = a / b;
            }
            else if (!strcmp(argv[i], "--build-asset"))
            {
                build_asset(requireArg(argv, i));
                quitAfterArgs = true;
                buildDeps = true;
            }
            else if (!strcmp(argv[i], "--quit"))
            {
                quitAfterArgs = true;
            }
            else if (!strcmp(argv[i], "--no-deps"))
            {
                noDeps = true;
            }
            else
            {
                goto unknown;
            }
        }
        else
        {
        unknown:
            throw std::runtime_error(std::string("Unknown argument: ") + argv[i]);
        }
    }
    if (buildDeps && !noDeps)
    {
        for (size_t i = 0; i != Reference::numBuiltItems(); ++i)
        {
            std::string assetName(Reference::builtItem(i));
            build_asset(assetName);
        }
    }
    if (quitAfterArgs)
    {
        fprintf(stderr, "Done.\n");
        exit(0);
    }
}


void mainLoop()
{
    try
    {
        while (running)
        {
            setupWindow(width, height, "raycar");
            Asset::init(GLContext::context());
            SceneGraph::init(GLContext::context());
            loadScene("content/raycar.scn");
            resetTimer();
            double now = 0;
            while (sceneRunning && running)
            {
                pollInput();
                double next = timer();
                if (active)
                {
                    if (next > now + maxStepTime)
                    {   //  falling behind
                        now = next - maxStepTime;
                    }
                    while (now < next)
                    {
                        stepScene();
                        now += stepTime;
                    }
                }
                else
                {
                    now = next;
                }
                renderWindow();
            }
        }
    }
    catch (std::exception const &x)
    {
        error(x.what());
    }
}



int main(int argc, char const *argv[])
{
    try
    {
        parseArgs(argc-1, argv+1);
    }
    catch (std::exception const &x)
    {
        error(std::string("Error during command option parsing: ") + x.what());
    }

    mainLoop();

    return 0;
}

#if WINDOWS_MAIN

int CALLBACK WinMain(
  __in  HINSTANCE hInstanceArg,
  __in  HINSTANCE hPrevInstance,
  __in  LPSTR lpCmdLine,
  __in  int nCmdShow
)
{
    ::SetErrorMode(0);
    hInstance = hInstanceArg;
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
    std::vector<char const *> argv;
    for (std::vector<std::string>::iterator ptr(argvstore.begin()), end(argvstore.end());
        ptr != end; ++ptr)
    {
        argv.push_back(const_cast<char *>((*ptr).c_str()));
    }
    argv.push_back(0);

    //  init GLUT
    int argc = argv.size() - 1;
    char const **argvp = &argv[0];

    return main(argc+1, argvp-1);
}

#endif

