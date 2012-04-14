
#include "rc_barrel.h"
#include "rc_config.h"
#include "rc_asset.h"
#include "rc_scenegraph.h"
#include "rc_model.h"

class BarrelBody : public OdeBody
{
public:
    BarrelBody(Barrel *b) :
        OdeBody(dBodyCreate(gWorld)),
        b_(b)
    {
        dMass m;
        dMassSetCylinderTotal(&m, b->mass_, 3, b->radius_, b->height_);
        dBodySetMass(id_, &m);
        Vec3 pos(b->pos());
        dBodySetPosition(id_, pos.x + b->center_.x, pos.y + b->center_.y, pos.z + b->center_.z);
        dBodySetData(id_, this);
    }
    Barrel *b_;
};

class BarrelGeom : public OdeGeom
{
public:
    BarrelGeom(Barrel *b) :
        OdeGeom(dCreateCylinder(gDynamicSpace, b->radius_, b->height_)),
        b_(b)
    {
        dGeomSetBody(id_, b->body_->id_);
        dGeomSetData(id_, this);
    }
    Barrel *b_;
};

Barrel::Barrel(Config const &cfg) :
    GameObject(cfg),
    node_(0),
    geom_(0),
    body_(0)
{
    name_ = cfg.string("name");
    modelName_ = cfg.string("model");
    model_ = Asset::model(modelName_);
    mass_ = cfg.numberf("mass");
    calcPhys(model_);
}

Barrel::~Barrel()
{
}

void Barrel::calcPhys(Model *m)
{
    Vec3 lob(m->lowerBound());
    Vec3 upb(m->upperBound());
    center_ = lob;
    addTo(center_, upb);
    scale(center_, 0.5f);
    subFrom(upb, lob);
    radius_ = std::max(upb.x, upb.y) * 0.5f;
    height_ = upb.z;
}

void Barrel::on_addToScene()
{
    node_ = SceneGraph::addModel(name_, model_);
    body_ = new BarrelBody(this);
    geom_ = new BarrelGeom(this);
}

void Barrel::on_removeFromScene()
{
    delete node_;
    node_ = 0;
    delete geom_;
    geom_ = 0;
    delete body_;
    body_ = 0;
}

void Barrel::on_step()
{
    Matrix x;
    body_->getTransform(x, -center_);
    node_->setTransform(x);
}
