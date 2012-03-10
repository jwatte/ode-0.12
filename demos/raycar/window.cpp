
#include "rc_window.h"
#include "rc_input.h"
#include "rc_context.h"
#include "rc_scene.h"

#include <GL/glew.h>

#if defined(_WINDOWS)
#include <Windows.h>
ATOM wClass;
HWND hWnd;
double timerMult;
double timerOffset;
extern HINSTANCE hInstance;
HDC hDC;
HGLRC hGLRC;
#else

#endif

#include <GL/glew.h>
#include <string>

bool running = true;
bool active = true;
double lastTimerValue = 0;

void error(std::string const&arg)
{
#if defined(_WINDOWS)
    ::MessageBoxA(0, arg.c_str(), "Error", MB_OK);
#else
    fprintf(stderr, "%s\n", arg.c_str());
#endif
    exit(1);
}

void error(int errcode, std::string const &arg)
{
    char str[200];
    ::FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM, 
        0,
        (DWORD)errcode, 
        0,
        str,
        200,
        0);
    ::error(arg + "\n" + str);
}


void inactivate()
{
    if (active)
    {
        lastTimerValue = timer();
        active = false;
        clearInput();
    }
}

void activate()
{
    if (!active)
    {
        active = true;
        setTimer(lastTimerValue);
    }
}



#if defined(_WINDOWS)
void setKeyInput(int vkey, bool to)
{
    switch (vkey)
    {
    case VK_UP:
    case 'W':
        setInput(ik_forward, to);
        break;
    case VK_DOWN:
    case 'S':
        setInput(ik_backward, to);
        break;
    case VK_LEFT:
    case 'A':
        setInput(ik_left, to);
        break;
    case VK_RIGHT:
    case 'D':
        setInput(ik_right, to);
        break;
    case VK_SPACE:
        setInput(ik_trigger, to);
        break;
    case VK_RETURN:
        setInput(ik_start, to);
        break;
    case VK_ESCAPE:
        setInput(ik_back, to);
        break;
    }
}

LRESULT CALLBACK WWndProc(
  __in  HWND hWnd,
  __in  UINT uMsg,
  __in  WPARAM wParam,
  __in  LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (active)
        {
            setKeyInput(wParam, true);
        }
        break;
    case WM_KEYUP:
        setKeyInput(wParam, false);
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hWnd, &ps);
            RECT r;
            ::GetWindowRect(hWnd, &r);
            ::EndPaint(hWnd, &ps);
        }
        return 0;
    case WM_ACTIVATE:
        if (wParam == 0)
        {
            inactivate();
        }
        else
        {
            activate();
        }
        break;
    case WM_CLOSE:
        running = false;
        break;
    case WM_CREATE:
        return TRUE;
    }
    return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void WRegisterClass()
{
    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    wcex.cbWndExtra = 0;
    wcex.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wcex.hCursor = ::LoadCursor(0, MAKEINTRESOURCE(IDC_CROSS));
    wcex.hInstance = hInstance;
    wcex.lpfnWndProc = &WWndProc;
    wcex.lpszClassName = L"raycar top-level window";
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wClass = ::RegisterClassExW(&wcex);
}

static void ctow(char const *cstr, wchar_t *wstr, size_t sz)
{
    for (size_t i = 0; i < sz; ++i)
    {
        wstr[i] = cstr[i];
        if (!wstr[i])
        {
            break;
        }
    }
    wstr[sz-1] = 0;
}

void WCreateHwnd(int width, int height, char const *title)
{
    wchar_t wtitle[100];
    ctow(title, wtitle, 100);

    hWnd = ::CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW,
        (LPCWSTR)wClass, 
        wtitle,
        WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU,
        96,
        32,
        width,
        height,
        0,
        0,
        hInstance,
        0);
    if (hWnd == 0)
    {
        error(::GetLastError(), "Could not initialize window.");
    }
    ::ShowWindow(hWnd, SW_NORMAL);
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        32,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                        //Number of bits for the depthbuffer
        8,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    hDC = ::GetDC(hWnd);
    int pf = ::ChoosePixelFormat(hDC, &pfd);
    if (!::SetPixelFormat(hDC, pf, &pfd))
    {
        error(::GetLastError(), "Could not initialize OpenGL.");
    }
    hGLRC = ::wglCreateContext(hDC);
    ::wglMakeCurrent(hDC, hGLRC);
    GLenum glewErr = glewInit();
    if (GLEW_OK != glewErr)
    {
        error("Graphics driver problem: cannot initialize GLEW.");
    }
    if (!GLEW_ARB_shader_objects || !GLEW_VERSION_2_0)
    {
        error("OpenGL driver does not support OpenGL 2.0 with shaders.");
    }
}

#endif


void resetTimer()
{
#if defined(_WINDOWS)
    long long tres;
    long long tnow;
    ::QueryPerformanceFrequency((LARGE_INTEGER *)&tres);
    ::QueryPerformanceCounter((LARGE_INTEGER *)&tnow);
    timerMult = 1.0 / (double)tres;
    timerOffset = -((double)tnow * timerMult);
#endif
}

double timer()
{
    if (!active)
    {
        return lastTimerValue;
    }
#if defined(_WINDOWS)
    long long tnow;
    ::QueryPerformanceCounter((LARGE_INTEGER *)&tnow);
    return ((double)tnow * timerMult) + timerOffset;
#endif
}

void setTimer(double value)
{
#if defined(_WINDOWS)
    long long tnow;
    ::QueryPerformanceCounter((LARGE_INTEGER *)&tnow);
    //  this is what it thinks it is
    double now = ((double)tnow * timerMult) + timerOffset;
    //  this is the delta to what I want
    timerOffset += value - now;
#endif
}

void setupWindow(int width, int height, char const *title)
{
#if defined(_WINDOWS)
    if (!wClass)
    {
        WRegisterClass();
    }
    if (!hWnd)
    {
        WCreateHwnd(width, height, title);
    }
    RECT r;
    ::GetWindowRect(hWnd, &r);
    GLContext::context()->realize(r.right, r.bottom);
    resetTimer();
#endif
}

void renderWindow()
{
#if defined(_WINDOWS)
    ::wglMakeCurrent(hDC, hGLRC);
#endif
    glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClearStencil(128);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    renderScene();

#if defined(_WINDOWS)
    ::SwapBuffers(hDC);
#endif
}

void pollInput()
{
    MSG msg;
    if (!active)
    {
        ::SleepEx(50, TRUE);
    }
    while (::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
}
