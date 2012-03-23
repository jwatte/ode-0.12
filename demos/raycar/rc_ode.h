
#if !defined(rc_ode_h)
#define rc_ode_h

#include <ode/ode.h>
extern dWorldID gWorld;
extern dSpaceID gDynamicSpace;
extern dSpaceID gStaticSpace;
extern dJointGroupID gJointGroup;

void physicsClear();
void physicsCreate();
void physicsStep();

class OdeBody;
class OdeGeom;
OdeBody *getBodyObj(dBodyID id);
OdeGeom *getGeomObj(dGeomID id);

class OdeBody
{
public:
    OdeBody(dBodyID id);
    virtual ~OdeBody();
    virtual bool onContact2(OdeBody *otherBody, OdeGeom *yourGeom, OdeGeom *otherGeom, dContact *contact);
    virtual bool onContact1(OdeBody *otherBody, OdeGeom *yourGeom, OdeGeom *otherGeom, dContact *contact);
    virtual void onStep();
    dBodyID id_;
};

class OdeGeom
{
public:
    OdeGeom(dGeomID id);
    virtual ~OdeGeom();
    virtual bool onContact2(OdeGeom *otherGeom, OdeBody *otherBody, dContact *contact);
    virtual bool onContact1(OdeGeom *otherGeom, OdeBody *otherBody, dContact *contact);
    inline dBodyID getBodyID() const { return dGeomGetBody(id_); }
    inline OdeBody *getBodyObj() const { return ::getBodyObj(getBodyID()); }
    dGeomID id_;
};

#endif  //  rc_ode_h
