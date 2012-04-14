
#if !defined(rc_box_h)
#define rc_box_h

#include "rc_gameobj.h"

struct Config;
class Model;
class SceneNode;

class Box : public GameObject
{
public:
    Box(Config const &cfg);
    ~Box();
    virtual void on_addToScene();
    virtual void on_removeFromScene();
    virtual void on_step();
private:
    friend class BoxBody;
    friend class BoxGeom;
    std::string name_;
    std::string modelName_;
    Model *model_;
    SceneNode *node_;
    Vec3 center_;
    Vec3 size_;
    float mass_;
    BoxBody *body_;
    BoxGeom *geom_;
};

#endif  //  rc_box_h
