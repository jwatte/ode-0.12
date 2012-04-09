
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
        Matrix m;
        cam.getModelView(this, &m);
        glLoadTransposeMatrixf(&m.rows[0][0]);
        mdl_->bind();
        if (bones_ != NULL)
        {
            size_t sz = 0;
            mdl_->bones(&sz);
            if (sz != boneCount_)
            {
                throw std::runtime_error("ModelSceneNode configured with wrong boneCount_");
            }
        }
        mdl_->issue(m, bones_);
    }
};

class SkyModelSceneNode : public ModelSceneNode
{
public:
    SkyModelSceneNode(std::string const &name, Model *mdl) :
        ModelSceneNode(name, mdl)
    {
        pass_ = 2;
    }
    virtual void render(CameraInfo const &cam)
    {
    glAssertError();
        Matrix m;
        cam.getModelView(this, &m);
        m.setTranslation(Vec3(0, 0, 0));
        glLoadTransposeMatrixf(&m.rows[0][0]);
        mdl_->bind();
        if (bones_ != NULL)
        {
            size_t sz = 0;
            mdl_->bones(&sz);
            if (sz != boneCount_)
            {
                throw std::runtime_error("ModelSceneNode configured with wrong boneCount_");
            }
        }
        mdl_->issue(m, bones_);
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
        cam_->mmat_.setTranslation(zero);
        cam_->mmat_.setRow(3, zero);
        subFrom(zero, pos());
        multiply(cam_->mmat_, zero);
        cam_->mmat_.setTranslation(zero);
    }
    virtual void render(CameraInfo const &cam)
    {
    }
};


SceneNode::SceneNode(std::string const &name) :
    xform_(Matrix::identity),
    name_(name),
    bones_(0),
    boneCount_(0),
    pass_(1)
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
    xform_.setTranslation(pos);
}

Matrix const &SceneNode::transform() const
{
    return xform_;
}

void SceneNode::setTransform(Matrix const &m)
{
    xform_ = m;
}

void SceneNode::setBones(::Bone const *bones, size_t count)
{
    bones_ = bones;
    boneCount_ = count;
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

SceneNode *SceneGraph::addSkyModel(std::string const &name, Model *mdl)
{
    ModelSceneNode *ret = new SkyModelSceneNode(name, mdl);
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
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT, GL_FILL);
    glAssertError();

    //  setup camera
    CameraInfo ci;
    camera->prepare(ci);
    ci = *static_cast<CameraSceneNode *>(camera)->cam_;

    glEnable(GL_LIGHT0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vec3 pos(-1, -2, 3);
    normalize(pos);
    Matrix m(ci.mmat_);
    m.setTranslation(Vec3(0, 0, 0));
    multiply(m, pos);
    glLightfv(GL_LIGHT0, GL_POSITION, &pos.x);

    Rgba ambLight(0.25f, 0.2f, 0.3f);
    glLightfv(GL_LIGHT0, GL_AMBIENT, &ambLight.r);
    Rgba diffLight(0.75f, 0.75f, 0.6f);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, &diffLight.r);
    Rgba specLight(0.75f, 0.75f, 0.6f);
    glLightfv(GL_LIGHT0, GL_SPECULAR, &diffLight.r);
    Rgba zero(0, 0, 0);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &zero.r);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    Vec3 sz(ctx_->size());
    float fy = tanf(ci.fovY / 360.0f * (float)3.1415927f);
    float fx = fy * sz.x / sz.y;
    glFrustum(-fx, fx, -fy, fy, 0.9f, 10000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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
    for (int pass = 1; pass < 3; ++pass)
    {
        for (std::set<SceneNode*>::iterator ptr(scene_.begin()), end(scene_.end());
            ptr != end; ++ptr)
        {
            if ((*ptr)->pass_ == pass && (*ptr) != camera)
            {
                (*ptr)->render(ci);
            }
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

Matrix SceneGraph::getCameraModelView(SceneNode *node)
{
    return static_cast<CameraSceneNode *>(node)->cam_->mmat_;
}

void CameraInfo::getModelView(SceneNode const *node, Matrix *omv) const
{
    multiply(mmat_, node != 0 ? node->transform() : Matrix::identity, *omv);
}
