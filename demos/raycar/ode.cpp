
#include "rc_ode.h"
#include "rc_math.h"
#include "rc_debug.h"

#include <ode/ode.h>
#include <set>


extern double stepTime;
static bool inited;

dWorldID gWorld;
dSpaceID gDynamicSpace;
dSpaceID gStaticSpace;
dJointGroupID gJointGroup;

static std::set<OdeBody *> bodies;



void physicsClear()
{
    if (gWorld)
    {
        dWorldDestroy(gWorld);
        dJointGroupDestroy(gJointGroup);
        dSpaceDestroy(gDynamicSpace);
        dSpaceDestroy(gStaticSpace);
        gWorld = 0;
    }
}

void physicsCreate()
{
    if (!inited)
    {
        dInitODE();
        inited = true;
    }
    gWorld = dWorldCreate();
    dWorldSetGravity(gWorld, 0, 0, -9.8f);
    gJointGroup = dJointGroupCreate(0);
    gDynamicSpace = dHashSpaceCreate(0);
    dHashSpaceSetLevels(gDynamicSpace, 2, 5);
    gStaticSpace = dHashSpaceCreate(0);
    dHashSpaceSetLevels(gStaticSpace, 2, 5);
}

static void nearCallback(void *, dGeomID o1, dGeomID o2)
{
    if (dGeomIsSpace(o1))
    {
        dSpaceCollide2(o1, o2, 0, &nearCallback);
        return;
    }
    if (dGeomIsSpace(o2))
    {
        dSpaceCollide2(o2, o1, 0, &nearCallback);
        return;
    }
    dContact cg[20];
    memset(cg, 0, sizeof(cg));
    int n = dCollide(o1, o2, 20, &cg[0].geom, (int)sizeof(dContact));
    for (int i = 0; i < n; ++i)
    {
        Vec3 posi(cg[i].geom.pos);
        Vec3 normi(cg[i].geom.normal);
        bool gotCopy = false;
        for (int j = 0; j < i; ++j)
        {
            Vec3 posj(cg[j].geom.pos);
            subFrom(posj, posi);
            //  contacts that are within 4" (0.1m) of each other are merged
            if (lengthSquared(posj) < 0.01f)
            {
                Vec3 normj(cg[j].geom.normal);
                if (dot(normi, normj) > 0.95f)
                {
                    //  normals similar -- got a similar contact
                    gotCopy = true;
                    break;
                }
            }
        }
        if (!gotCopy)
        {
            cg[i].surface.mu = 0.5f;
            cg[i].surface.mode = dContactApprox1;
            dBodyID b1 = dGeomGetBody(o1);
            dBodyID b2 = dGeomGetBody(o2);
            OdeBody *bp1 = b1 ? (OdeBody *)dBodyGetData(b1) : 0;
            OdeBody *bp2 = b2 ? (OdeBody *)dBodyGetData(b2) : 0;
            OdeGeom *gp1 = (OdeGeom *)dGeomGetData(o1);
            OdeGeom *gp2 = (OdeGeom *)dGeomGetData(o2);
            if (bp2)
                if (!bp2->onContact2(bp1, gp2, gp1, &cg[i]))
                    continue;
            if (bp1)
                if (!bp1->onContact1(bp2, gp1, gp2, &cg[i]))
                    continue;
            if (gp2)
                if (!gp2->onContact2(gp1, bp1, &cg[i]))
                    continue;
            if (gp1)
                if (!gp1->onContact1(gp2, bp2, &cg[i]))
                    continue;
            dJointID jid = dJointCreateContact(gWorld, gJointGroup, &cg[i]);
            dJointAttach(jid, dGeomGetBody(o1), dGeomGetBody(o2));
            addDebugLine(Vec3(cg[i].geom.pos), Vec3(cg[i].geom.normal), Rgba(1, 0, 0, 1));
        }
    }
}

void physicsStep()
{
    dJointGroupEmpty(gJointGroup);
    dSpaceCollide2((dGeomID)gDynamicSpace, (dGeomID)gStaticSpace, 0, &nearCallback);
    dSpaceCollide(gDynamicSpace, 0, &nearCallback);
    for (std::set<OdeBody *>::iterator ptr(bodies.begin()), end(bodies.end());
        ptr != end; ++ptr)
    {
        (*ptr)->onStep();
    }
    dWorldQuickStep(gWorld, (float)stepTime);
}

OdeBody *getBodyObj(dBodyID id)
{
    return id ? (OdeBody *)dBodyGetData(id) : 0;
}

OdeGeom *getGeomObj(dGeomID id)
{
    return id ? (OdeGeom *)dGeomGetData(id) : 0;
}

OdeBody::OdeBody(dBodyID id) :
    id_(id)
{
    bodies.insert(this);
}

OdeBody::~OdeBody()
{
    bodies.erase(bodies.find(this));
}

bool OdeBody::onContact2(OdeBody *otherBody, OdeGeom *yourGeom, OdeGeom *otherGeom, dContact *contact)
{
    return true;
}

bool OdeBody::onContact1(OdeBody *otherBody, OdeGeom *yourGeom, OdeGeom *otherGeom, dContact *contact)
{
    return true;
}

void OdeBody::onStep()
{
}


OdeGeom::OdeGeom(dGeomID id) :
    id_(id)
{
}

OdeGeom::~OdeGeom()
{
}

bool OdeGeom::onContact2(OdeGeom *otherGeom, OdeBody *otherBody, dContact *contact)
{
    return true;
}

bool OdeGeom::onContact1(OdeGeom *otherGeom, OdeBody *otherBody, dContact *contact)
{
    return true;
}
