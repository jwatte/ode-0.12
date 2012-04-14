
#if !defined(rc_barrel_h)
#define rc_barrel_h

#include "rc_gameobj.h"
#include "rc_ode.h"

#include <vector>

struct Config;
class Model;
class SceneNode;

class Barrel : public GameObject
{
public:
    Barrel(Config const &cfg);
    ~Barrel();
    virtual void on_addToScene();
    virtual void on_removeFromScene();
    virtual void on_step();
private:
    friend class BarrelBody;
    friend class BarrelGeom;
    void calcPhys(Model *m);
    std::string name_;
    std::string modelName_;
    float mass_;
    float height_;
    float radius_;
    Vec3 center_;
    Model *model_;
    SceneNode *node_;
    BarrelGeom *geom_;
    BarrelBody *body_;
};

#endif  //  rc_barrel_h
