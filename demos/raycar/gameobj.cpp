
#include "rc_gameobj.h"
#include "rc_config.h"
#include "rc_model.h"
#include "rc_ixfile.h"
#include "rc_context.h"
#include "rc_asset.h"
#include "rc_scenegraph.h"
#include "rc_car.h"
#include "rc_obstacle.h"

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

class FollowCamera : public GameObject
{
public:
    FollowCamera(Config const &c) : GameObject(c)
    {
        camInfo_.fovY = c.numberf("fovy");
        Vec3 cPos = Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z"));
        setPos(cPos);
        camInfo_.lookAt.x = cPos.x + 1;
        camInfo_.lookAt.y = cPos.y;
        camInfo_.lookAt.z = cPos.z;
        name_ = c.string("name");
        target_ = c.string("target");
        distance_ = c.numberf("distance");
    }
    CameraInfo camInfo_;
    SceneNode *node_;
    std::string name_;
    std::string target_;
    float distance_;
    void on_addToScene()
    {
        node_ = SceneGraph::addCamera(name_, &camInfo_);
        node_->setPos(pos());
    }
    void on_removeFromScene()
    {
        delete node_;
        node_ = 0;
    }
    void on_step()
    {
        Vec3 cPos(pos());
        SceneNode *target = SceneGraph::nodeNamed(target_);
        if (target) {
            Matrix tx(target->transform());
            Vec3 tPos(tx.translation());
            camInfo_.lookAt.x = tPos.x;
            camInfo_.lookAt.y = tPos.y;
            camInfo_.lookAt.z = tPos.z + 1; //  hack!
            Vec3 d(tx.getColumn(1));
            scale(d, -distance_);
            addTo(tPos, Vec3(0, 0, distance_ * 0.25f)); //  hack!
            subFrom(tPos, d);
            subFrom(tPos, cPos);
            scale(tPos, 0.1f);
            if (fabs(tPos.z) < 0.5f) {
                tPos.z = 0;
            }
            else {
                tPos.z = tPos.z * 2;    //  find height sooner
            }
            addTo(cPos, tPos);
            setPos(cPos);
        }
        node_->setPos(pos());
    }
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
GameObjectTypeImpl<FollowCamera> gtFollowCamera;
GameObjectTypeImpl<Car> gtCar;

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
    else if (type == "followcamera")
    {
        return &gtFollowCamera;
    }
    else if (type == "car")
    {
        return &gtCar;
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

