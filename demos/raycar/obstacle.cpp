
#include "rc_obstacle.h"
#include "rc_asset.h"
#include "rc_config.h"
#include "rc_scenegraph.h"
#include "rc_model.h"


Obstacle::Obstacle(Config const &c) : GameObject(c)
{
    modelName_ = c.string("model");
    model_ = Asset::model(modelName_);
    setPos(Vec3(c.numberf("x"), c.numberf("y"), c.numberf("z")));
    name_ = c.string("name");
}

Obstacle::~Obstacle()
{
    delete model_;
    model_ = 0;
}

void Obstacle::on_addToScene()
{
    node_ = SceneGraph::addModel(name_, model_);
    size_t batchCnt = 0;
    TriangleBatch const *triBatch = model_->batches(&batchCnt);
    size_t boneCnt = 0;
    Bone const *bone = model_->bones(&boneCnt);
    size_t vSize = model_->vertexSize();
    char const * vertex = (char const *)model_->vertices();
    unsigned int const *index = (unsigned int const *)model_->indices();
    for (size_t bi = 0; bi != batchCnt; ++bi)
    {
        dTriMeshDataID tmd = dGeomTriMeshDataCreate();
        dGeomTriMeshDataBuildSingle(tmd, vertex, vSize, triBatch[bi].maxVertexIndex + 1, 
            index + triBatch[bi].firstTriangle * 3, triBatch[bi].numTriangles * 3, 12);
        dGeomID geom = dCreateTriMesh(gStaticSpace, tmd, 0, 0, 0);
        tmd_.push_back(tmd);
        geom_.push_back(geom);
        Matrix bx;
        get_bone_transform(bone, triBatch[bi].bone, bx);
        Vec3 p(bx.translation());
        addTo(p, pos());
        dGeomSetPosition(geom, p.x, p.y, p.z);
        dGeomSetRotation(geom, bx.rows[0]);
    }
}

void Obstacle::on_removeFromScene()
{
    delete node_;
    node_ = 0;
    for (std::vector<dGeomID>::iterator gptr(geom_.begin()), gend(geom_.end());
        gptr != gend; ++gptr)
    {
        dSpaceRemove(gStaticSpace, *gptr);
        dGeomDestroy(*gptr);
    }
    for (std::vector<dTriMeshDataID>::iterator dptr(tmd_.begin()), dend(tmd_.end());
        dptr != dend; ++dptr)
    {
        dGeomTriMeshDataDestroy(*dptr);
    }
}

void Obstacle::on_step()
{
    node_->setPos(pos());
}
