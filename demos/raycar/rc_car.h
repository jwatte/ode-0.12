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
    void setPos(Vec3 const &pos);
    void setTransform(Matrix const &m);
    Matrix const &transform() const { return transform_; }

private:
    friend class CarBody;
    friend class CarChassis;

    Model *model_;
    SceneNode *node_;
    std::string name_;
    dBodyID body_;
    dGeomID chassis_;
    Vec3 offset_;
    CarBody *bodyObj_;
    CarChassis *chassisObj_;
    dGeomID wheelRay_;
    Matrix transform_;

    int lastBump_;          //  steps between bumps
    bool bump_;             //  try to un-stick
    bool canBump_;          //  has had ground contact
    float steer_;           //  input control to steering
    float gas_;             //  input control to propulsion
    float topSpeed_;        //  how fast does it go at top speed?
    float steerDamping_;    //  how sluggish is the steering?
    float brakeFriction_;   //  how sluggish is the steering?
    float steerFriction_;   //  how sluggish is the steering?
    float sideSlip_;        //  how much do we slip sideways?
    float suspensionCfm_;   //  spring slop
    float suspensionErp_;   //  spring force
    float enginePower_;     //  how fast it can accelerate
    float airDrag_;         //  how much air drag by velocity squared
    float speedSteer_;      //  how much to allow steering at speed
    float wheelBase_;       //  distance from front to back axle
    float wheelWidth_;      //  distance from left to right front wheel
    float wheelTurn_[4];    //  how much the wheel has turned
    float wheelExtent_[4];  //  bottom edge of wheel in Z
    float wheelNeutral_[4]; //  bottom edge of wheel in Z when at rest
    unsigned int wheelBone_[4]; //  Bones indices used for rendering; maps to well-known order
    std::vector<Bone> bones_;   //  Bone data used for rendering
    Vec3 wheelCenter_[4];   //  the car-local position of each wheel bone when in neutral position. Also, wheelNeutral - wheelCenter.z == wheel radius
    Vec3 steerLeft_;
    Vec3 steerRight_;
};

#endif
