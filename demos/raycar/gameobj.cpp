
#include "rc_gameobj.h"
#include "rc_config.h"
#include "rc_model.h"
#include "rc_ixfile.h"
#include "rc_context.h"
#include "rc_asset.h"
#include "rc_scenegraph.h"

class Camera : public GameObject
{
public:
    Camera(Config const &c) : GameObject(c)
    {
        camInfo_.fovY = c.numberf("fovy");
        setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
        camInfo_.lookAt.x = c.numberf("lx");
        camInfo_.lookAt.y = c.numberf("ly");
        camInfo_.lookAt.z = c.numberf("lz");
        name_ = c.string("name");
    }
    void on_addToScene()
    {
        node_ = SceneGraph::addCamera(name_, &camInfo_);
    }
    void on_removeFromScene()
    {
        delete node_;
        node_ = 0;
    }
    CameraInfo camInfo_;
    SceneNode *node_;
    std::string name_;
    void on_step()
    {
        node_->setPos(pos());
    }
};

class Obstacle : public GameObject
{
public:
    Obstacle(Config const &c) : GameObject(c)
    {
        modelName_ = c.string("model");
        model_ = Asset::model(modelName_);
        setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
        name_ = c.string("name");
    }
    ~Obstacle()
    {
        delete model_;
        model_ = 0;
    }
    void on_addToScene()
    {
        node_ = SceneGraph::addModel(name_, model_);
    }
    void on_removeFromScene()
    {
        delete node_;
        node_ = 0;
    }
    void on_step()
    {
        node_->setPos(pos());
    }
    std::string modelName_;
    Model *model_;
    SceneNode *node_;
    std::string name_;
};

template<typename T> class GameObjectTypeImpl : public GameObjectType
{
public:
    GameObject *instantiate(Config const &cfg)
    {
        return new T(cfg);
    }
};

GameObjectTypeImpl<Obstacle> gtObstacle;
GameObjectTypeImpl<Camera> gtCamera;

GameObjectType *GameObjectType::type(std::string const &type)
{
    if (type == "obstacle")
    {
        return &gtObstacle;
    }
    else if (type == "camera")
    {
        return &gtCamera;
    }
    else
    {
        throw std::runtime_error(std::string("Unknown game object type: ") + type);
    }
}

GameObject::GameObject(Config const &cfg)
{
    pos_.x = cfg.numberf("x");
    pos_.y = cfg.numberf("y");
    pos_.z = cfg.numberf("z");
}

GameObject::~GameObject()
{
}

void GameObject::on_addToScene()
{
}

void GameObject::on_removeFromScene()
{
}

void GameObject::on_step()
{
}

Vec3 const &GameObject::pos() const
{
    return pos_;
}

void GameObject::setPos(Vec3 const &pos)
{
    pos_ = pos;
}

