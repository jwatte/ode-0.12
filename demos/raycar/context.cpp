
#include "rc_context.h"

#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <stdexcept>
#include <string>

void glAssertError_(char const *file, int line)
{
    uint32_t err = glGetError();
    if (err != GL_NO_ERROR)
    {
        char buf[200];
        _snprintf_s(buf, 200, "%s:%d:\nGraphics error: 0x%x", file, line, err);
        throw std::runtime_error(std::string(buf));
    }
}

static GLContext ctx;

GLContext *GLContext::context()
{
    return &ctx;
}

void GLContext::realize(int width, int height)
{
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);
    glDepthRange(0.0f, 1.0f);
}

Vec3 GLContext::size()
{
    return Vec3((float)width_, (float)height_, 1.0f);
}