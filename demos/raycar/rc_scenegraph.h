
#if !defined(rc_scenegraph_h)
#define rc_scenegraph_h

#include "rc_math.h"
#include <assert.h>

class GLContext;
class Model;
class Camera;
class SceneNode;
struct CameraInfo;
struct Bone;

class SceneGraph
{
public:
    static void init(GLContext *ctx);
    static SceneNode *addModel(std::string const &name, Model *mdl);
    static SceneNode *addSkyModel(std::string const &name, Model *mdl);
    static SceneNode *addCamera(std::string const &name, CameraInfo *cam);
    static void present(SceneNode *camera);
    static SceneNode *nodeNamed(std::string const &name);
    static Matrix getCameraModelView(SceneNode *node);
};

class SceneNode
{
public:
    SceneNode(std::string const &name);
    virtual ~SceneNode();
    Vec3 worldPos() const;
    void setPos(Vec3 const &pos);
    Matrix const &transform() const;
    void worldTransform(Matrix &oMat) const;
    void setTransform(Matrix const &m);
    virtual void setBones(Bone const *bones, size_t cnt);
    std::string const &name() const;
    void setParent(SceneNode *sn);
    SceneNode *parent() const;
    template<typename T> T *as() {
#if !defined(NDEBUG)
        assert(dynamic_cast<T *>(this) != NULL);
#endif
        return static_cast<T *>(this);
    }
    template<typename T> T const *as() const {
#if !defined(NDEBUG)
        assert(dynamic_cast<T const *>(this) != NULL);
#endif
        return static_cast<T const *>(this);
    }
protected:
    friend class SceneGraph;
    virtual void prepare(CameraInfo const &cam) = 0;
    virtual void render(CameraInfo const &cam, int pass) = 0;
    Matrix xform_;
    std::string name_;
    Bone const *bones_;
    size_t boneCount_;
    int pass_;
    SceneNode *parent_;
};

struct CameraInfo
{
    Vec3 lookAt;
    Vec3 back;
    float fovY;

    void getModelView(SceneNode const *node, Matrix *omv) const;
    //  calculated by prepare()
    Matrix mmat_;
};

enum SceneNodePass
{
    p_opaque = 1,
    p_sky_box = 2,
    p_transparent = 4,
    p_num_passes = 3
};

#endif  //  rc_scenegraph_h
