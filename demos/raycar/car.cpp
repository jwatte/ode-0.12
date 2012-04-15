
#include "rc_car.h"
#include "rc_model.h"
#include "rc_scenegraph.h"
#include "rc_asset.h"
#include "rc_config.h"
#include "rc_input.h"
#include "rc_debug.h"

#include <assert.h>


class Car;


class CarBody : public OdeBody
{
public:
    CarBody(Car *c, dBodyID id) : OdeBody(id), car_(c) {}
    void onStep();
    Car *car_;
    dContactGeom nearest_; //  distance of ray to surface
    Vec3 rayOrigin_;        //  used while hit testing

    static void wheelRayCallback(void *data, dGeomID o1, dGeomID o2);
};

class CarChassis : public OdeGeom
{
public:
    CarChassis(Car *c, dGeomID id) : OdeGeom(id), car_(c) {}
    Car *car_;
    bool onContact2(OdeGeom *otherGeom, OdeBody *otherBody, dContact *contact)
    {
        car_->canBump_ = true;
        contact->surface.mu = 0.5f;
        return true;
    }
    bool onContact1(OdeGeom *otherGeom, OdeBody *otherBody, dContact *contact)
    {
        car_->canBump_ = true;
        contact->surface.mu = 0.5f;
        return true;
    }
};


Car::Car(Config const &c) : GameObject(c)
{
    node_ = NULL;
    std::string modelName = c.string("model");
    model_ = Asset::model(modelName);
    setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
    name_ = c.string("name");
    body_ = 0;
    chassis_ = 0;
    bodyObj_ = 0;
    lastBump_ = 0;
    bump_ = false;
    canBump_ = false;
    chassisObj_ = 0;
    memset(wheelExtent_, 0, sizeof(wheelExtent_));
    memset(wheelTurn_, 0, sizeof(wheelTurn_));
    memset(wheelBone_, 0, sizeof(wheelBone_));
    topSpeed_ = c.numberf("topSpeed");
    steerDamping_ = c.numberf("steerDamping");
    brakeFriction_ = c.numberf("brakeFriction");
    steerFriction_ = c.numberf("steerFriction");
    sideSlip_ = c.numberf("sideSlip");
    suspensionCfm_ = c.numberf("suspensionCfm");
    suspensionErp_ = c.numberf("suspensionErp");
    enginePower_ = c.numberf("enginePower");
    airDrag_ = c.numberf("airDrag");
    speedSteer_ = c.numberf("speedSteer");
    steer_ = 0;
    gas_ = 0;
    size_t bCount;
    Bone const *b = model_->bones(&bCount);
    bones_.insert(bones_.end(), b, b + bCount);
}

Car::~Car()
{
    delete model_;
}

void Car::on_addToScene()
{
    node_ = SceneGraph::addModel(name_, model_);

    wheelRay_ = dCreateRay(0, 1.0f);
    body_ = dBodyCreate(gWorld);
    bodyObj_ = new CarBody(this, body_);
    dBodySetData(body_, bodyObj_);
    dMass m;
    Bone const *b = model_->boneNamed("chassis");
    if (!b)
    {
        throw std::runtime_error("Did not find chassis bone in car.");
    }
    size_t boneCnt = 0;
    Bone const *skel = model_->bones(&boneCnt);
    Vec3 lower = b->lowerBound;
    Vec3 upper = b->upperBound;
    Matrix xMat;
    get_bone_transform(skel, b-skel, xMat);
    multiply(xMat, lower);
    multiply(xMat, upper);
    Vec3 offset(upper);
    Vec3 size(upper);
    addTo(offset, lower);
    subFrom(size, lower);   //  size is extent
    scale(offset, 0.5f);    //  offset is center
    dMassSetBoxTotal(&m, 100, size.x, size.y, size.z);
    dBodySetMass(body_, &m);
    dBodySetAutoDisableFlag(body_, false);
    chassis_ = dCreateBox(gDynamicSpace, size.x, size.y, size.z);
    chassisObj_ = new CarChassis(this, chassis_);
    dGeomSetData(chassis_, chassisObj_);
    dGeomSetBody(chassis_, body_);
    dGeomSetOffsetPosition(chassis_, offset.x, offset.y, offset.z);
    Vec3 p(pos());
    dBodySetPosition(body_, p.x, p.y, p.z);
    static char const *names[] =
    {
        "wheel_fl",
        "wheel_rl",
        "wheel_fr",
        "wheel_rr",
    };
    int found = 0;
    float wx[4];
    float wy[4];
    for (size_t i = 0; i < 4; ++i)
    {
        b = model_->boneNamed(names[i]);
        if (!b)
        {
            throw std::runtime_error(std::string("Could not find wheel bone named ") + names[i] + " in car model.");
        }
        get_bone_transform(skel, b-skel, xMat);
        Vec3 wPos(b->lowerBound);
        multiply(xMat, wPos);
        wx[i] = xMat.translation().x;
        wy[i] = xMat.translation().y;
        wheelExtent_[i] = wPos.z;  //  wheelExtent -- where bottom surface of wheel is
        wheelNeutral_[i] = wheelExtent_[i]; //  wheelNeutral -- where I want the wheel bottom to be
        multiply(xMat, wPos);
        wheelCenter_[i] = xMat.translation();  //  wheelCenter -- where the center of the wheel is
        wheelBone_[i] = b - skel;
    }
    wheelBase_ = wy[1] - wy[0];
    wheelWidth_ = wx[0] - wx[2];
}

void Car::on_removeFromScene()
{
    dSpaceRemove(gDynamicSpace, chassis_);
    dGeomDestroy(wheelRay_);
    dGeomDestroy(chassis_);
    dBodyDestroy(body_);
    delete bodyObj_;
    bodyObj_ = 0;
    delete chassisObj_;
    chassisObj_ = 0;
    delete node_;
    node_ = 0;
}

void Car::setPos(Vec3 const &pos)
{
    Matrix m(transform_);
    m.setTranslation(pos);
    setTransform(m);
}

void Car::setTransform(Matrix const &m)
{
    transform_ = m;
    if (node_) {    //  note: I may be out of the scene
        node_->setTransform(m);
    }
    //  what to do about body_ ?
    GameObject::setPos(m.translation());
}

void Car::on_step()
{
    /* update node transform based on rigid body */
    Vec3 p(dBodyGetPosition(body_));
    if (p.z < -100) {
        dBodySetPosition(body_, 0, 0, 5);
        dBodySetLinearVel(body_, 0, 0, 0);
        dBodySetAngularVel(body_, 0, 0, 0);
        float q[4] = { 1, 0, 0, 0 };
        dBodySetQuaternion(body_, q);
    }
    float const *rot = dBodyGetRotation(body_);
    Matrix m(Matrix::identity);
    m.setRow(0, rot);
    m.setRow(1, rot + 4);
    m.setRow(2, rot + 8);
    m.setTranslation(p);
    setTransform(m);
    //  tripod
    addDebugLine(p, m.right(), Rgba(1, 0, 0, 1));
    addDebugLine(p, m.forward(), Rgba(0, 1, 0, 1));
    addDebugLine(p, m.up(), Rgba(0, 0, 1, 1));

    /* update the wheels based on extent */
    for (size_t i = 0; i < 4; ++i)
    {
        //  todo: apply steering rotation
        //  todo: apply wheel rotation
        //  wheelNeutral - wheelCenter means bottom of wheel
        //  translation of wheel should be (wheelCenter - (wheelNeutral - wheelExtent))
        //  so if neutral > extent, wheel extends down
        (*(Matrix *)bones_[wheelBone_[i]].xform).rows[2][3] = wheelCenter_[i].z - (wheelNeutral_[i] - wheelExtent_[i]);
    }
    node_->setBones(&bones_[0], bones_.size());

    //  todo: brake is not a recognized control right now
    float gas = 0, brake = 0, steer = 0;
    if (!getAnalogInput(gas, brake, steer))
    {
        /* parse gas/brake input */
        if (testInput(ik_backward)) {
            gas_ = gas_ - 0.05f;
        }
        else if (testInput(ik_forward)) {
            gas_ = gas_ + 0.01f;
        }
        else {
            if (gas_ > 0) {
                gas_ = gas_ - 0.01f;
            }
            else if (gas_ < 0) {
                gas_ = gas_ + 0.02f;
            }
            if (gas_ > -0.02f && gas_ < 0.02f) {
                gas_ = 0;
            }
        }

        /* parse left/right input */
        float sd = 0;
        if (testInput(ik_left)) {
            sd -= 1;
        }
        if (testInput(ik_right)) {
            sd += 1;
        }
        if (sd == 0) {
            if (steer_ > 0) {
                steer_ -= 0.1f;
            }
            else if (steer_ < 0) {
                steer_ += 0.1f;
            }
            if (steer_ >= -0.1f && steer_ <= 0.1f) {
                steer_ = 0;
            }
        }
        else {
            steer_ = sd - (sd - steer_) * steerDamping_;
        }
    }
    else
    {
        gas_ = gas - brake;
        steer_ = steer;
    }
    if (gas_ > 1) {
        gas_ = 1;
    }
    if (gas_ < -0.5f) {
        gas_ = -0.5f;
    }
    if (steer_ > 1) {
        steer_ = 1;
    }
    else if (steer_ < -1) {
        steer_ = -1;
    }
    bump_ = false;
    if (lastBump_ > 0) {
        --lastBump_;
    }
    if (canBump_ && lastBump_ == 0 && testInput(ik_trigger)) {
        bump_ = true;
        lastBump_ = 100;
    }
}

//  Walk through wheels, testing one at a time for contact with ground.
//  This emulates suspension AND propulsion AND steering through the use 
//  of contact constraints.
void CarBody::onStep()
{
    Vec3 vel = *(Vec3 const *)dBodyGetLinearVel(car_->body_);
    //  the model comes out backward, because it's modeled facing the viewer, so this is what it is
    Matrix cXform(car_->transform());
    Vec3 fwd = cXform.backward();
    Vec3 up = cXform.up();
    Vec3 right = cXform.left();
    float fVel = dot(vel, fwd);
    float steerScale = 1.0f / std::max(1.0f, fabsf(fVel) / car_->speedSteer_);
    //  air drag
    float vee2 = length(vel);
    float vee = sqrtf(vee2);
    //  for convenience, the linear and vee-squared terms have the same coefficient
    float airDrag = car_->airDrag_;
    dBodyAddForce(car_->body_, -vel.x * (1 + vee) * airDrag, -vel.y * (1 + vee) * airDrag, -vel.z * (1 + vee) * airDrag);
    //  steer based on inputs
    Vec3 steerFwd(right);
    float actualSteer = car_->steer_ * steerScale;
    scale(steerFwd, actualSteer);
    addTo(steerFwd, fwd);
    normalize(steerFwd);
    //  Ackerman steering!
    Vec3 steerLeft(steerFwd), steerRight(steerFwd);
    if (actualSteer > 0.01f) {
        //  ackerman adjust left
        float ratio = dot(steerFwd, right);
        assert(ratio > 0);
        float alpha = 1.0f / (1 + ratio);
        scale(steerLeft, alpha);
        Vec3 fwdBit(fwd);
        scale(fwdBit, 1-alpha);
        addTo(steerLeft, fwdBit);
        normalize(steerLeft);
    }
    else if (actualSteer < -0.01f) {
        //  ackerman right
        float ratio = -dot(steerFwd, right);
        assert(ratio > 0);
        float alpha = 1.0f / (1 + ratio);
        scale(steerRight, alpha);
        Vec3 fwdBit(fwd);
        scale(fwdBit, 1-alpha);
        addTo(steerRight, fwdBit);
        normalize(steerRight);
    }
    car_->steerLeft_ = steerLeft;
    car_->steerRight_ = steerRight;
    float gasForce = car_->gas_ * car_->enginePower_;
    if (fVel > car_->topSpeed_ * 0.5f) {
        gasForce = gasForce * car_->topSpeed_ * 0.5f / fVel;
    }
    float braking = std::min(1.0f, std::max(0.f, 1 - fabsf(car_->gas_ * 5)));
    //  For each of the car wheel positions, fire a ray in the car "down" direction.
    //  Put the wheel at that point if it hits something, else put the wheel at the end.
    for (size_t i = 0; i != 4; ++i)
    {
        //  for each wheel, set a ray to the center of the wheel bone position
        Vec3 pos = car_->wheelCenter_[i];
        multiply(car_->transform(), pos);
        rayOrigin_ = pos;
        // from the center of the wheel bone, in the "down" direction of the car
        dGeomRaySet(car_->wheelRay_, pos.x, pos.y, pos.z, -up.x, -up.y, -up.z);
        // I want the closest collision for trimesh/ray intersections
        dGeomRaySetClosestHit(car_->wheelRay_, 1);
        //  max extent is one full wheel radius down -- neutral is one wheel radius already, so 2 for max
        float maxExtent = fabs(car_->wheelNeutral_[i] - car_->wheelCenter_[i].z) * 2;
        // wheel can move at most half a wheel radius, plus the distance from center to radius at rest
        dGeomRaySetLength(car_->wheelRay_, maxExtent);
        dGeomRaySetParams(car_->wheelRay_, 0, 0);   // firstContact, backfaceCull
        nearest_.depth = 10000; //  Detect if there's no contact

        // Now, collide this wheel ray with the world to sense contact
        dSpaceCollide2((dGeomID)gStaticSpace, car_->wheelRay_, this, &CarBody::wheelRayCallback);
        if (nearest_.depth > maxExtent)
        {
            //  maxExtent means the wheel suspension is maximally extended,
            //  and there is no contact
            nearest_.depth = maxExtent;
        }
        //  extend the wheel bottoms to the point of contact
        car_->wheelExtent_[i] = car_->wheelCenter_[i].z - nearest_.depth;
        if (nearest_.depth < maxExtent) // not <=, because == means "no contact"
        {
            //  got a contact
            dContact c;
            memset(&c, 0, sizeof(c));
            c.geom = nearest_;
            //  penetration depth is depth from furthest contact point
            //  -- this means I have to carefully tweak the CFM/ERP for "sink-in"
            c.geom.depth = maxExtent * 0.5f - nearest_.depth;
            Vec3 cpos(*(Vec3 *)c.geom.pos);
            Vec3 cdir(*(Vec3 *)c.geom.normal);
            Vec3 cpos2;
            subFrom(cpos2, cdir);
            bool contact = false;
            if (c.geom.depth > -0.05f) {    //  some "skin depth" to keep contact with road when suspension extended a little bit
                //  I can bump if there's been a contact since the last bump
                car_->canBump_ = true;
                contact = true;
                addDebugLine(cpos, cpos2, Rgba(1, 0, 1, 1));

                c.geom.normal[0] *= -1;
                c.geom.normal[1] *= -1;
                c.geom.normal[2] *= -1;
                //  figure out which direction the wheels turn
                //  frontleft and frontright are steering wheels
                if (i == 2) {
                    memcpy(c.fdir1, &steerRight, sizeof(steerRight));
                }
                else if (i == 0) {
                    memcpy(c.fdir1, &steerLeft, sizeof(steerLeft));
                }
                else {
                    //  driving wheels
                    memcpy(c.fdir1, &fwd, sizeof(fwd));
                    if (contact) {
                        dBodyAddForceAtPos(car_->body_, 
                            c.fdir1[0] * -gasForce, c.fdir1[1] * -gasForce, c.fdir1[2] * -gasForce, 
                            c.geom.pos[0], c.geom.pos[1], c.geom.pos[2]);
                    }
                }
                if (contact) {
                    addDebugLine(*(Vec3 *)c.geom.pos, *(Vec3 *)c.fdir1, Rgba(1.0f, 0.5f, 0.0f, 1.0f));
                }
                //  Turn on a bunch of features of the contact joint, used to emulate wheels and steering.
                c.surface.mode = dContactApprox1 | dContactMu2 | dContactSlip1 | dContactSlip2 |
                    dContactSoftERP | dContactSoftCFM | dContactFDir1;
                //  While rolling, there's no friction in the "mu" (fdir1) direction.
                c.surface.mu = car_->brakeFriction_ * braking;
                //  There's always friction in the sideways direction
                c.surface.mu2 = car_->steerFriction_;
                c.surface.slip1 = 0;
                //  some amount of sideways slip (fds -- force dependent slip)
                c.surface.slip2 = car_->sideSlip_;
                //  suspension coefficients for bounciness
                c.surface.soft_cfm = car_->suspensionCfm_;
                c.surface.soft_erp = car_->suspensionErp_;
                //  create the joint even if the suspension is not yet touching the ground,
                //  because I want to avoid penetration
                dJointID jid = dJointCreateContact(gWorld, gJointGroup, &c);
                dJointAttach(jid, car_->body_, 0);
            }
        }
    }
    if (car_->bump_) {
        dBodyAddForce(car_->body_, steerFwd.x * 10000, steerFwd.y * 10000, 100000);
        dBodyAddRelTorque(car_->body_, 0, 10000, 0);
        car_->canBump_ = false;
        car_->bump_ = false;
    }
}

void CarBody::wheelRayCallback(void *data, dGeomID o1, dGeomID o2)
{
    CarBody *cb = (CarBody *)data;
    //  wheels don't hit the car proper
    if (cb->id_ == dGeomGetBody(o1) || cb->id_ == dGeomGetBody(o2))
    {
        return;
    }
    if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
    {
        dSpaceCollide2(o1, o2, data, &CarBody::wheelRayCallback);
        return;
    }
    dContactGeom cg[20];
    size_t nc = dCollide(o1, o2, 10, cg, sizeof(dContactGeom));
    for (size_t i = 0; i != nc; ++i)
    {
        Vec3 pos(cg[i].pos);
        subFrom(pos, cb->rayOrigin_);
        float depth = length(pos);
        if (depth < cb->nearest_.depth)
        {
            cb->nearest_ = cg[i];
            cb->nearest_.depth = depth;
        }
    }
}
