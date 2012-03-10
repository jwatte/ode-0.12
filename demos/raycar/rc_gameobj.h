#if !defined(rc_gameobj_h)
#define rc_gameobj_h

#include <list>
#include "rc_math.h"

struct Config;
class GameObject;
class GameComponent;

class GameObjectType
{
public:
    static GameObjectType *type(std::string const &name);
    virtual GameObject *instantiate(Config const &cfg) = 0;
};

class GameObject
{
public:
    virtual void on_addToScene();
    virtual void on_removeFromScene();
    virtual void on_step();
    Vec3 const &pos() const;
    void setPos(Vec3 const &pos);
protected:
    friend void deleteSceneObject(GameObject *obj);
    GameObject(Config const &cfg);
    virtual ~GameObject();
private:
    GameObject();
    Vec3 pos_;
};


#endif  //  rc_gameobj_h
