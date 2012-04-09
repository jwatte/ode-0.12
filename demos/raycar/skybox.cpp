
#include "rc_skybox.h"
#include "rc_config.h"
#include "rc_asset.h"
#include "rc_scenegraph.h"

Skybox::Skybox(Config const &cfg) :
    GameObject(cfg)
{
    name_ = cfg.string("name");
    modelName_ = cfg.string("model");
    model_ = Asset::model(modelName_);
}

Skybox::~Skybox()
{
}

void Skybox::on_addToScene()
{
    node_ = SceneGraph::addSkyModel(name_, model_);
}

void Skybox::on_removeFromScene()
{
    delete node_;
    node_ = 0;
}

