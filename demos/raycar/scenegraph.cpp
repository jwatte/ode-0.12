
#include "rc_scenegraph.h"
#include "rc_context.h"
#include "rc_model.h"

#include <GL/GLEW.h>

#include <string>
#include <set>
#include <algorithm>
#include <assert.h>


static std::set<SceneNode *> scene_;
static GLContext *ctx_;

class CompareNearFar
{
public:
    int operator()(std::pair<float, SceneNode *> const &a, std::pair<float, SceneNode *> const &b) const
    {
        float d = a.first - b.first;
        return d > 0;
    }
};

class CompareFarNear
{
public:
    int operator()(std::pair<float, SceneNode *> const &a, std::pair<float, SceneNode *> const &b) const
    {
        float d = b.first - a.first;
        return d > 0;
    }
};

class ModelBatchSceneNode : public SceneNode
{
public:
    ModelBatchSceneNode(std::string const &name, Model *mdl, size_t batch) :
        SceneNode(name),
        mdl_(mdl),
        batch_(batch)
    {
        pass_ = batchIsTransparent(mdl, batch) ? p_transparent : p_opaque;
        scene_.insert(this);
    }
    Model *mdl_;
    size_t batch_;
    virtual void prepare(CameraInfo const &cam)
    {
    }
    virtual void render(CameraInfo const &cam, int pass)
    {
        Matrix m;
        cam.getModelView(this, &m);
        glLoadTransposeMatrixf((float *)m.rows);
        mdl_->bind();
        mdl_->issueBatch(m, batch_, bones_);
        glAssertError();
    }
    static bool batchIsTransparent(Model *mdl, size_t batch)
    {
        size_t cnt = 0;
        TriangleBatch const *b = mdl->batches(&cnt);
        assert(batch < cnt);
        Material const *mtl = mdl->materials(&cnt);
        assert(b[batch].material < cnt);
        return mtl[b[batch].material].maps[mk_opacity].name[0] != 0;
    }
};

class ModelSceneNode : public SceneNode
{
public:
    ModelSceneNode(std::string const &name, Model *mdl) :
        SceneNode(name),
        mdl_(mdl)
    {
        pass_ = 0;
        size_t cnt = 0;
        TriangleBatch const *batches = mdl->batches(&cnt);
        for (size_t i = 0; i != cnt; ++i)
        {
            char buf[1024];
            _snprintf_s(buf, 1024, "%s:batch-%d", name.c_str(), i);
            //  all model batch scene nodes are relative to me
            ModelBatchSceneNode *mbsn = new ModelBatchSceneNode(buf, mdl, i);
            mbsn->setParent(this);
            batchNodes_.push_back(mbsn);
        }
    }
    ~ModelSceneNode()
    {
        for (std::vector<ModelBatchSceneNode *>::iterator
            ptr(batchNodes_.begin()),
            end(batchNodes_.end());
            ptr != end;
            ++ptr)
        {
            delete *ptr;
        }
    }
    Model *mdl_;
    std::vector<ModelBatchSceneNode *> batchNodes_;
    virtual void prepare(CameraInfo const &cam)
    {
        size_t sz = 0;
        Bone const *mBones = mdl_->bones(&sz);
        if (bones_ != NULL)
        {
            if (sz != boneCount_)
            {
                throw std::runtime_error("ModelSceneNode configured with wrong boneCount_");
            }
            mBones = bones_;
        }
        size_t cnt;
        TriangleBatch const *mtbs = mdl_->batches(&cnt);
        for (std::vector<ModelBatchSceneNode *>::iterator
            ptr(batchNodes_.begin()),
            end(batchNodes_.end());
            ptr != end;
            ++ptr)
        {
            ModelBatchSceneNode *mbsn = *ptr;
            TriangleBatch const &tb = mtbs[mbsn->batch_];
            Matrix m;
            get_bone_transform(mBones, tb.bone, m);
            mbsn->setTransform(m);
        }
    }
    virtual void render(CameraInfo const &cam, int pass)
    {
        assert(!"ModelSceneNode is not directly renderable");
    }
};

class SkyModelSceneNode : public SceneNode
{
public:
    SkyModelSceneNode(std::string const &name, Model *mdl) :
        SceneNode(name)
    {
        pass_ = p_sky_box;
        mdl_ = mdl;
    }
    Model *mdl_;
    virtual void prepare(CameraInfo const &cam)
    {
        if (bones_ != NULL)
        {
            size_t sz = 0;
            mdl_->bones(&sz);
            if (sz != boneCount_)
            {
                throw std::runtime_error("ModelSceneNode configured with wrong boneCount_");
            }
        }
    }
    virtual void render(CameraInfo const &cam, int pass)
    {
    glAssertError();
        Matrix m;
        cam.getModelView(this, &m);
        m.setTranslation(Vec3(0, 0, 0));
        mdl_->bind();
        size_t cnt;
        TriangleBatch const *tb = mdl_->batches(&cnt);
        for (size_t i = 0; i != cnt; ++i)
        {
            mdl_->issueBatch(m, i, bones_);
        }
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
        Vec3 wPos(worldPos());
        Vec3 back(wPos);
        subFrom(back, cam_->lookAt);
        normalize(back);
        cam_->back = back;
        Vec3 up(0, 0, 1);
        Vec3 right;
        cross(right, up, back);
        normalize(right);
        cross(up, back, right);
        cam_->mmat_.setRow(0, right);
        cam_->mmat_.setRow(1, up);
        cam_->mmat_.setRow(2, back);
        Vec3 zPos;
        cam_->mmat_.setTranslation(zPos);
        cam_->mmat_.setRow(3, zPos);
        subFrom(zPos, wPos);
        multiply(cam_->mmat_, zPos);
        cam_->mmat_.setTranslation(zPos);
    }
    virtual void render(CameraInfo const &cam, int pass)
    {
    }
};


SceneNode::SceneNode(std::string const &name) :
    xform_(Matrix::identity),
    name_(name),
    bones_(0),
    boneCount_(0),
    pass_(p_opaque),
    parent_(0)
{
}

SceneNode::~SceneNode()
{
    scene_.erase(scene_.find(this));
}

Vec3 SceneNode::worldPos() const
{
    Matrix wx;
    worldTransform(wx);
    return wx.translation();
}

void SceneNode::setPos(Vec3 const &pos)
{
    xform_.setTranslation(pos);
}

Matrix const &SceneNode::transform() const
{
    return xform_;
}

void SceneNode::worldTransform(Matrix &oMat) const
{
    if (!parent_) {
        oMat = xform_;
        return;
    }
    parent_->worldTransform(oMat);
    multiply(oMat, xform_, oMat);
}

void SceneNode::setTransform(Matrix const &m)
{
    xform_ = m;
}

void SceneNode::setParent(SceneNode *parent)
{
    parent_ = parent;
}

SceneNode *SceneNode::parent() const
{
    return parent_;
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
    SkyModelSceneNode *ret = new SkyModelSceneNode(name, mdl);
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
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glAssertError();

    //  setup camera
    CameraInfo ci;
    camera->prepare(ci);
    ci = *camera->as<CameraSceneNode>()->cam_;

    glEnable(GL_LIGHT0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vec3 pos(1, 2, 3);
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
    static std::vector<std::pair<float, SceneNode *> > toDraw;
    toDraw.reserve(8000);
    Vec3 camBack(ci.back);

    for (int pno = 0; pno < p_num_passes; ++pno)
    {
        int pass = (1 << pno);
        toDraw.clear();
        for (std::set<SceneNode*>::iterator ptr(scene_.begin()), end(scene_.end());
            ptr != end; ++ptr)
        {
            if (((*ptr)->pass_ & pass) != 0)
            {
                toDraw.push_back(std::pair<float, SceneNode *>(dot((*ptr)->worldPos(), camBack), *ptr));
            }
        }
        if (pass == p_transparent)
        {
            //  transparency is sorted back-to-front
            std::sort(toDraw.begin(), toDraw.end(), CompareFarNear());
        }
        else
        {
            //  everything else is sorted front-to-back, to get early z rejection
            std::sort(toDraw.begin(), toDraw.end(), CompareNearFar());
        }
        for (std::vector<std::pair<float, SceneNode *> >::iterator
            ptr(toDraw.begin()), end(toDraw.end());
            ptr != end;
            ++ptr)
        {
            (*ptr).second->render(ci, pass);
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
    if (node != 0) {
        Matrix wx;
        node->worldTransform(wx);
        multiply(mmat_, wx, *omv);
    }
    else
    {
        *omv = mmat_;
    }
}
