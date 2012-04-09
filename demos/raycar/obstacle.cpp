
#include "rc_obstacle.h"
#include "rc_asset.h"
#include "rc_config.h"
#include "rc_scenegraph.h"
#include "rc_model.h"


Obstacle::Obstacle(Config const &c) : GameObject(c)
{
    modelName_ = c.string("model");
    model_ = Asset::model(modelName_);
    setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
    name_ = c.string("name");
    geom_ = 0;
}

Obstacle::~Obstacle()
{
    delete model_;
    model_ = 0;
}

void Obstacle::on_addToScene()
{
    node_ = SceneGraph::addModel(name_, model_);
    //  todo: move trimesh data id into Model proper to share it
    tmd_ = dGeomTriMeshDataCreate();
    //  todo: transform trimesh data by bones!
    ...
    dGeomTriMeshDataBuildSingle(tmd_, model_->vertices(), model_->vertexSize(), 
        model_->vertexCount(), model_->indices(), model_->indexCount(), 12);
    geom_ = dCreateTriMesh(gStaticSpace, tmd_, 0, 0, 0);
    Vec3 p(pos());
    dGeomSetPosition(geom_, p.x, p.y, p.z);
}

void Obstacle::on_removeFromScene()
{
    delete node_;
    node_ = 0;
    dSpaceRemove(gStaticSpace, geom_);
    dGeomDestroy(geom_);
    dGeomTriMeshDataDestroy(tmd_);
}

void Obstacle::on_step()
{
    node_->setPos(pos());
}
