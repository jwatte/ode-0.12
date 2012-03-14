
#if !defined(rc_scenegraph_h)
#define rc_scenegraph_h

#include "rc_math.h"

class GLContext;
class Model;
class Camera;
class SceneNode;
struct CameraInfo;

class SceneGraph
{
public:
    static void init(GLContext *ctx);
    static SceneNode *addModel(std::string const &name, Model *mdl);
    static SceneNode *addCamera(std::string const &name, CameraInfo *cam);
    static void present(SceneNode *camera);
    static SceneNode *nodeNamed(std::string const &name);
    static Matrix getCameraModelView(SceneNode *node);
};

class SceneNode
{
public:
    SceneNode(std::string const &name);
    ~SceneNode();
    Vec3 pos() const;
    void setPos(Vec3 const &pos);
    Matrix const &transform() const;
    void setTransform(Matrix const &m);
    std::string const &name() const;
protected:
    friend class SceneGraph;
    virtual void prepare(CameraInfo const &cam) = 0;
    virtual void render(CameraInfo const &cam) = 0;
    Matrix xform_;
    std::string name_;
};

struct CameraInfo
{
    Vec3 lookAt;
    float fovY;

    void getModelView(SceneNode const *node, Matrix *omv) const;
    //  calculated by prepare()
    Matrix mmat_;
};

#endif  //  rc_scenegraph_h
