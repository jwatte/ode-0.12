
#include "rc_scenegraph.h"
#include "rc_context.h"
#include "rc_model.h"

#include <string>
#include <set>
#include <GL/GLEW.h>


static std::set<SceneNode *> scene_;
static GLContext *ctx_;


class ModelSceneNode : public SceneNode
{
public:
    ModelSceneNode(std::string const &name, Model *mdl) :
        SceneNode(name),
        mdl_(mdl)
    {
    }
    Model *mdl_;
    virtual void prepare(CameraInfo const &cam)
    {
    }
    virtual void render(CameraInfo const &cam)
    {
    glAssertError();
        glMatrixMode(GL_MODELVIEW);
        Matrix m;
        cam.getModelView(this, &m);
        mdl_->bind();
        mdl_->issue(m);
    }
};

class CameraSceneNode : public SceneNode
{
public:
    CameraSceneNode(std::string const &name, CameraInfo *cam) :
        SceneNode(name),
        cam_(cam)
    {
    }
    CameraInfo *cam_;
    virtual void prepare(CameraInfo const &cam)
    {
        Vec3 back(pos());
        subFrom(back, cam_->lookAt);
        normalize(back);
        Vec3 up(0, 0, 1);
        Vec3 right;
        cross(right, up, back);
        normalize(right);
        cross(up, back, right);
        cam_->mmat_.setRow(0, right);
        cam_->mmat_.setRow(1, up);
        cam_->mmat_.setRow(2, back);
        Vec3 zero;
        cam_->mmat_.setRow(3, zero);
        subFrom(zero, pos());
        cam_->mmat_.setTranslation(zero);
    }
    virtual void render(CameraInfo const &cam)
    {
    }
};


SceneNode::SceneNode(std::string const &name) :
    xform_(Matrix::identity),
    name_(name)
{
}

SceneNode::~SceneNode()
{
    scene_.erase(scene_.find(this));
}

Vec3 SceneNode::pos() const
{
    return Vec3(xform_.rows[0][3], xform_.rows[1][3], xform_.rows[2][3]);
}

void SceneNode::setPos(Vec3 const &pos)
{
    xform_.rows[0][3] = pos.x;
    xform_.rows[1][3] = pos.y;
    xform_.rows[2][3] = pos.z;
}

Matrix const &SceneNode::transform() const
{
    return xform_;
}

std::string const &SceneNode::name() const
{
    return name_;
}

void SceneGraph::init(GLContext *ctx)
{
    ctx_ = ctx;
}

SceneNode *SceneGraph::addModel(std::string const &name, Model *mdl)
{
    ModelSceneNode *ret = new ModelSceneNode(name, mdl);
    scene_.insert(ret);
    return ret;
}

SceneNode *SceneGraph::addCamera(std::string const &name, CameraInfo *cam)
{
    CameraSceneNode *ret = new CameraSceneNode(name, cam);
    scene_.insert(ret);
    return ret;
}

void SceneGraph::present(SceneNode *camera)
{
    glAssertError();

    //  setup camera
    CameraInfo ci;
    camera->prepare(ci);
    ci = *static_cast<CameraSceneNode *>(camera)->cam_;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    Vec3 sz(ctx_->size());
    float fy = tanf(ci.fovY / 360.0f * (float)3.1415927f);
    float fx = fy * sz.x / sz.y;
    glFrustum(-fx, fx, -fy, fy, 1.0f, 100000.0f);
    glMatrixMode(GL_MODELVIEW);
    glAssertError();

    //  prepare all objects
    for (std::set<SceneNode*>::iterator ptr(scene_.begin()), end(scene_.end());
        ptr != end; ++ptr)
    {
        if ((*ptr) != camera)
        {
            (*ptr)->prepare(ci);
        }
    }

    //  render all objects
    for (std::set<SceneNode*>::iterator ptr(scene_.begin()), end(scene_.end());
        ptr != end; ++ptr)
    {
        if ((*ptr) != camera)
        {
            (*ptr)->render(ci);
        }
    }

    glAssertError();
}

SceneNode *SceneGraph::nodeNamed(std::string const &findName)
{
    for (std::set<SceneNode *>::iterator ptr(scene_.begin()), end(scene_.end());
        ptr != end; ++ptr)
    {
        if ((*ptr)->name() == findName)
        {
            return *ptr;
        }
    }
    return NULL;
}


void CameraInfo::getModelView(SceneNode const *node, Matrix *omv) const
{
    Vec3 const &pos(node->pos());
    *omv = node->transform();
    multiply(mmat_, *omv);
}
