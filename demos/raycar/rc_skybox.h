
#if !defined(rc_skybox_h)
#define rc_skybox_h

#include "rc_gameobj.h"

struct Config;
class Model;
class SceneNode;

class Skybox : public GameObject
{
public:
    Skybox(Config const &cfg);
    ~Skybox();
    virtual void on_addToScene();
    virtual void on_removeFromScene();
private:
    std::string name_;
    std::string modelName_;
    Model *model_;
    SceneNode *node_;
};

#endif  //  rc_skybox_h
