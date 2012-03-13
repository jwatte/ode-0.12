
#if !defined(rc_obstacle_h)
#define rc_obstacle_h

#include "rc_gameobj.h"
#include "rc_math.h"
#include "rc_ode.h"

#include <string>

class Model;
struct Config;
class SceneNode;

class Obstacle : public GameObject
{
public:
    Obstacle(Config const &c);
    ~Obstacle();
    void on_addToScene();
    void on_removeFromScene();
    void on_step();

    std::string modelName_;
    Model *model_;
    SceneNode *node_;
    std::string name_;
    dTriMeshDataID tmd_;
    dGeomID geom_;
};

#endif  //  rc_obstacle
