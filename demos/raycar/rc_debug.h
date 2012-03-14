
#if !defined(rc_debug_h)
#define rc_debug_h

#include "rc_math.h"

class SceneNode;

void addDebugLine(Vec3 const &from, Vec3 const &dir, Rgba const &color);
void clearDebugLines();
void drawDebugLines(SceneNode *camera);

#endif
