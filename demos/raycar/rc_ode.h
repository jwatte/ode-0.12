
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

#endif  //  rc_ode_h
