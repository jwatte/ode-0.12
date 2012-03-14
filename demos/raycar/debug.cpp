
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
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
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
    glDisable(GL_COLOR_MATERIAL);
glAssertError();
    GLContext::context()->endCustom();
}

void clearDebugLines()
{
    debugLines.clear();
}
