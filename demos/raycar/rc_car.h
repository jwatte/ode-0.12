#if !defined(rc_car_h)

#include "rc_gameobj.h"
#include "rc_ode.h"

class Model;
class SceneNode;

class Car : public GameObject
{
public:
    Car(Config const &c);
    ~Car();
    virtual void on_addToScene();
    virtual void on_removeFromScene();
    virtual void on_step();

private:
    Model *model_;
    SceneNode *node_;
    std::string name_;
    dBodyID body_;
    dGeomID chassis_;
};

#endif
