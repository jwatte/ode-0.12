
#include "rc_car.h"
#include "rc_model.h"
#include "rc_scenegraph.h"
#include "rc_asset.h"
#include "rc_config.h"



Car::Car(Config const &c) : GameObject(c)
{
    std::string modelName = c.string("model");
    model_ = Asset::model(modelName);
    setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
    name_ = c.string("name");
    body_ = 0;
    chassis_ = 0;
}

Car::~Car()
{
    delete model_;
    model_ = 0;
}

void Car::on_addToScene()
{
    node_ = SceneGraph::addModel(name_, model_);

    body_ = dBodyCreate(gWorld);
    dMass m;
    dMassSetBoxTotal(&m, 100, 2, 6, 1);
    dBodySetMass(body_, &m);
    dBodySetAutoDisableFlag(body_, false);
    dBodySetData(body_, (GameObject *)this);
    chassis_ = dCreateBox(gDynamicSpace, 2, 6, 1);
    dGeomSetData(chassis_, (GameObject *)this);
    dGeomSetBody(chassis_, body_);
    Vec3 p(pos());
    dBodySetPosition(body_, p.x, p.y, p.z);
}

void Car::on_removeFromScene()
{
    dSpaceRemove(gDynamicSpace, chassis_);
    dGeomDestroy(chassis_);
    dBodyDestroy(body_);

    delete node_;
    node_ = 0;
}

void Car::on_step()
{
    Vec3 p(dBodyGetPosition(body_));
    float const *rot = dBodyGetRotation(body_);
    Matrix m(Matrix::identity);
    m.setRow(0, rot);
    m.setRow(1, rot + 4);
    m.setRow(2, rot + 8);
    m.setTranslation(p);
    node_->setTransform(m);
}

