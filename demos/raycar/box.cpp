
#include "rc_box.h"
#include "rc_config.h"
#include "rc_asset.h"
#include "rc_scenegraph.h"
#include "rc_ode.h"
#include "rc_model.h"

class BoxBody : public OdeBody
{
public:
    BoxBody(Box *b) :
        OdeBody(dBodyCreate(gWorld)),
        b_(b)
    {
        dMass m;
        dMassSetBoxTotal(&m, b->mass_, b->size_.x, b->size_.y, b->size_.z);
        dBodySetMass(id_, &m);
        Vec3 pos(b->pos());
        dBodySetPosition(id_, pos.x + b->center_.x, pos.y + b->center_.y, pos.z + b->center_.z);
        dBodySetData(id_, this);
    }
    Box *b_;
};

class BoxGeom : public OdeGeom
{
public:
    BoxGeom(Box *b) :
        OdeGeom(dCreateBox(gDynamicSpace, b->size_.x, b->size_.y, b->size_.z)),
        b_(b)
    {
        dGeomSetBody(id_, b->body_->id_);
        dGeomSetData(id_, this);
    }
    Box *b_;
};

Box::Box(Config const &cfg) :
    GameObject(cfg)
{
    name_ = cfg.string("name");
    modelName_ = cfg.string("model");
    model_ = Asset::model(modelName_);
    center_ = model_->lowerBound();
    addTo(center_, model_->upperBound());
    scale(center_, 0.5f);
    size_ = model_->upperBound();
    subFrom(size_, model_->lowerBound());
    mass_ = cfg.numberf("mass");
}

Box::~Box()
{
}

void Box::on_addToScene()
{
    node_ = SceneGraph::addModel(name_, model_);
    body_ = new BoxBody(this);
    geom_ = new BoxGeom(this);
}

void Box::on_removeFromScene()
{
    delete node_;
    node_ = 0;
    delete geom_;
    geom_ = 0;
    delete body_;
    body_ = 0;
}

void Box::on_step()
{
    Matrix x;
    body_->getTransform(x, -center_);
    node_->setTransform(x);
}
