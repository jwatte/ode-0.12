
#include "rc_car.h"
#include "rc_model.h"
#include "rc_scenegraph.h"
#include "rc_asset.h"
#include "rc_config.h"
#include "rc_input.h"


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
};


Car::Car(Config const &c) : GameObject(c)
{
    std::string modelName = c.string("model");
    model_ = Asset::model(modelName);
    setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
    name_ = c.string("name");
    body_ = 0;
    chassis_ = 0;
    bodyObj_ = 0;
    chassisObj_ = 0;
    memset(wheelExtent_, 0, sizeof(wheelExtent_));
    memset(wheelTurn_, 0, sizeof(wheelTurn_));
    memset(wheelBone_, 0, sizeof(wheelBone_));
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
    Vec3 lower = b->lowerBound;
    Vec3 upper = b->upperBound;
    Vec3 offset(upper);
    Vec3 size(upper);
    addTo(offset, lower);
    subFrom(size, lower);
    scale(offset, 0.5f);
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
    for (size_t i = 0; i < 4; ++i)
    {
        b = model_->boneNamed(names[i]);
        wheelExtent_[i] = b->lowerBound.z;
        wheelNeutral_[i] = wheelExtent_[i];
        wheelCenter_[i] = (*(Matrix const *)b->xform).translation().z;
        for (size_t j = 0; j < bones_.size(); ++j)
        {
            if (!strcmp(bones_[j].name, names[i]))
            {
                wheelBone_[i] = j;
                ++found;
                break;
            }
        }
    }
    if (found != 4)
    {
        throw std::runtime_error("Did not find all four wheel bones in car.");
    }
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

void Car::on_step()
{
    /* update node transform based on rigid body */
    Vec3 p(dBodyGetPosition(body_));
    float const *rot = dBodyGetRotation(body_);
    Matrix m(Matrix::identity);
    m.setRow(0, rot);
    m.setRow(1, rot + 4);
    m.setRow(2, rot + 8);
    m.setTranslation(p);
    node_->setTransform(m);

    /* update the wheels based on extent */
    for (size_t i = 0; i < 4; ++i)
    {
        //  todo: apply steering rotation
        (*(Matrix *)bones_[wheelBone_[i]].xform).rows[2][3] = wheelExtent_[i] - (wheelNeutral_[i] - wheelCenter_[i]);
    }
    node_->setBones(&bones_[0], bones_.size());

    /* parse gas/brake input */
    if (testInput(ik_backward)) {
        gas_ = gas_ - 0.05f;
        if (gas_ < -1) {
            gas_ = -1;
        }
    }
    else if (testInput(ik_forward)) {
        gas_ = gas_ + 0.01f;
        if (gas_ > 1) {
            gas_ = 1;
        }
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
        else if (steer_ >= -0.1f && steer_ <= 0.1f) {
            steer_ = 0;
        }
    }
    else {
        steer_ = steer_ + sd * 0.1f;
        if (steer_ > 1) {
            steer_ = 1;
        }
        else if (steer_ < -1) {
            steer_ = -1;
        }
    }
}

void CarBody::onStep()
{
    //  For each of the car wheel positions, fire a ray in the car "down" direction.
    //  Put the wheel at that point if it hits something, else put the wheel at the end.
    Vec3 fwd = car_->node_->transform().getColumn(1);
    Vec3 up = car_->node_->transform().getColumn(2);
    Vec3 right = car_->node_->transform().getColumn(0);
    Vec3 steerFwd(right);
    scale(steerFwd, car_->steer_);
    addTo(steerFwd, fwd);
    normalize(steerFwd);
    for (size_t i = 0; i != 4; ++i)
    {
        //  for each wheel, set the ray to the center of the wheel bone position
        Vec3 pos = (*(Matrix const *)car_->bones_[car_->wheelBone_[i]].xform).translation();
        pos.z = car_->wheelCenter_[i];
        multiply(car_->node_->transform(), pos);
        rayOrigin_ = pos;
        // from the center of the wheel bone, in the "down" direction of the car
        dGeomRaySet(car_->wheelRay_, pos.x, pos.y, pos.z, -up.x, -up.y, -up.z);
        // I want the closest collision for trimesh/ray intersections
        dGeomRaySetClosestHit(car_->wheelRay_, 1);
        float maxExtent = fabs(car_->wheelNeutral_[i] - car_->wheelCenter_[i]) * 2;
        // wheel can move at most half a wheel radius, plus the distance from center to radius at rest
        dGeomRaySetLength(car_->wheelRay_, maxExtent);
        dGeomRaySetParams(car_->wheelRay_, 0, 0);   // firstContact, backfaceCull
        nearest_.depth = 10000; //  Detect if there's no contact

        // Now, collide this wheel ray with the world to sense contact
        dSpaceCollide2((dGeomID)gStaticSpace, car_->wheelRay_, this, &CarBody::wheelRayCallback);
        if (nearest_.depth > maxExtent)
        {
            nearest_.depth = maxExtent;
        }
        //  extend the wheels to the point of contact
        car_->wheelExtent_[i] = car_->wheelCenter_[i] - nearest_.depth;
        if (nearest_.depth < maxExtent) // not <=, because == means "no contact"
        {
            //  got a contact
            dContact c;
            memset(&c, 0, sizeof(c));
            c.geom = nearest_;
            c.geom.depth = (car_->wheelCenter_[i] - car_->wheelNeutral_[i]) - nearest_.depth + 0.1f; // hack!
            if (c.geom.depth > 0) {
                c.geom.normal[0] *= -1;
                c.geom.normal[1] *= -1;
                c.geom.normal[2] *= -1;
                //  frontleft and frontright are steering wheels
                if (i == 0 || i == 2) {
                    memcpy(c.fdir1, &steerFwd, sizeof(steerFwd));
                }
                else {
                    memcpy(c.fdir1, &fwd, sizeof(fwd));
                }
                c.surface.mode = dContactApprox1 | dContactMu2 | dContactSlip2 | dContactMotion1 |
                    dContactSoftERP | dContactSoftCFM | dContactFDir1;
                c.surface.mu = 1.0f;
                c.surface.mu2 = 0.5f;
                c.surface.slip2 = 0.001f;
                c.surface.soft_cfm = 0.002f;
                c.surface.soft_erp = 0.3f;
                //  doing this in ray intersection gives us traction only when a wheel touches the ground
                c.surface.motion1 = -car_->gas_ * 10;
                dJointID jid = dJointCreateContact(gWorld, gJointGroup, &c);
                dJointAttach(jid, car_->body_, 0);
            }
        }
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
