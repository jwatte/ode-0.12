
#include "rc_debug.h"
#include "rc_context.h"
#include "rc_scenegraph.h"

#include <GL/GLEW.h>
#include <list>

struct DebugLine
{
    Vec3 from_;
    Vec3 dir_;
    Rgba color_;
};

static std::list<DebugLine> debugLines;

void addDebugLine(Vec3 const &from, Vec3 const &dir, Rgba const &color)
{
    DebugLine dl = { from, dir, color };
    debugLines.push_back(dl);
}

void drawDebugLines(SceneNode *camera)
{
    GLContext::context()->beginCustom(SceneGraph::getCameraModelView(camera));
    glBegin(GL_LINES);
    for (std::list<DebugLine>::iterator ptr(debugLines.begin()), end(debugLines.end());
        ptr != end; ++ptr)
    {
        glColor4fv(&(*ptr).color_.r);
        Vec3 pos((*ptr).from_);
        glVertex3fv(&pos.x);
        addTo(pos, (*ptr).dir_);
        glVertex3fv(&pos.x);
    }
    glEnd();
glAssertError();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    for (std::list<DebugLine>::iterator ptr(debugLines.begin()), end(debugLines.end());
        ptr != end; ++ptr)
    {
        float f[4];
        memcpy(f, &(*ptr).color_.r, 4 * sizeof(float));
        f[3] = 0.5f;
        glColor4fv(f);
        Vec3 pos((*ptr).from_);
        glVertex3fv(&pos.x);
        addTo(pos, (*ptr).dir_);
        glVertex3fv(&pos.x);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
glAssertError();
    GLContext::context()->endCustom();
}

void clearDebugLines()
{
    debugLines.clear();
}
