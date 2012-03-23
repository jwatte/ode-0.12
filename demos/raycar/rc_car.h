#if !defined(rc_car_h)

#include "rc_gameobj.h"
#include "rc_ode.h"
#include "rc_math.h"
#include "rc_model.h"

#include <vector>

class Model;
class SceneNode;
class CarBody;
class CarChassis;

class Car : public GameObject
{
public:
    Car(Config const &c);
    ~Car();
    virtual void on_addToScene();
    virtual void on_removeFromScene();
    virtual void on_step();

private:
    friend class CarBody;
    friend class CarGeom;

    Model *model_;
    SceneNode *node_;
    std::string name_;
    dBodyID body_;
    dGeomID chassis_;
    Vec3 offset_;
    CarBody *bodyObj_;
    CarChassis *chassisObj_;
    dGeomID wheelRay_;

    float steer_;           //  input control to steering
    float gas_;             //  input control to propulsion
    float wheelTurn_[4];    //  how much the wheel has turned
    float wheelExtent_[4];  //  bottom edge of wheel in Z
    float wheelNeutral_[4]; //  bottom edge of wheel in Z when at rest
    unsigned int wheelBone_[4]; //  Bones indices used for rendering; maps to well-known order
    std::vector<Bone> bones_;   //  Bone data used for rendering
    float wheelCenter_[4];  //  the Z value of each wheel bone when in neutral position. Also, wheelNeutral - wheelCenter == wheel radius
};

#endif
