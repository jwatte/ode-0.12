
#include <GL/glew.h>

#include "rc_model.h"
#include "rc_context.h"
#include "rc_ixfile.h"


enum BinMagic
{
    bm_magicFile = 0x01007e8e,
    bm_magicVertices,
    bm_magicIndices,
    bm_magicLayout,
    bm_magicBatches,
    bm_magicMaterials,
    bm_magicBones
};


VertexLayout VertPtn::layout_[3] =
{
    { vc_position, 0, 3, GL_FLOAT },
    { vc_texture0, 12, 2, GL_FLOAT },
    { vc_normal, 20, 3, GL_FLOAT },
};

void readBuffer(IxRead *file, std::vector<char> &buffer)
{
    uint32_t size;
    file->read(&size, 4);
    if (size > MAX_FILE_SIZE)
    {
        throw std::runtime_error("Invalid bin file format");
    }
    buffer.resize(size);
    file->read(&buffer[0], size);
}

void writeBuffer(IxWrite *file, void const *data, size_t size)
{
    if (size > MAX_FILE_SIZE)
    {
        throw std::runtime_error("Attempt to write too large file");
    }
    uint32_t u32 = (uint32_t)size;
    file->write(&u32, 4);
    file->write(data, u32);
}

void read_bin(IxRead *file, IModelData *m)
{
    uint32_t magic;
    file->read(&magic, 4);
    if (magic != bm_magicFile)
    {
        throw std::runtime_error("Not a proper binary model file");
    }
    while (file->pos() < file->size())
    {
        file->read(&magic, 4);
        std::vector<char> buffer;
        uint32_t val;
        switch (magic)
        {
        case bm_magicVertices:
            file->read(&val, 4);
            readBuffer(file, buffer);
            m->setVertexData(&buffer[0], buffer.size(), val);
            break;
        case bm_magicIndices:
            file->read(&val, 4);
            readBuffer(file, buffer);
            m->setIndexData(&buffer[0], buffer.size(), val);
            break;
        case bm_magicBatches:
            readBuffer(file, buffer);
            m->setBatches((TriangleBatch const *)&buffer[0], buffer.size() / sizeof(TriangleBatch));
            break;
        case bm_magicMaterials:
            readBuffer(file, buffer);
            m->setMaterials((Material const *)&buffer[0], buffer.size() / sizeof(Material));
            break;
        case bm_magicBones:
            readBuffer(file, buffer);
            m->setBones((Bone const *)&buffer[0], buffer.size() / sizeof(Bone));
            break;
        case bm_magicLayout:
            readBuffer(file, buffer);
            m->setVertexLayout((VertexLayout *)&buffer[0], buffer.size() / sizeof(VertexLayout));
            break;
        default:
            throw std::runtime_error("Unknown bin model file chunk type");
        }
    }
    delete file;
}

Model::~Model()
{
    releaseMaterials();
    glDeleteBuffers(1, &vertices_);
    glDeleteBuffers(1, &indices_);
}

Model *Model::readFromFile(IxRead *file, GLContext *ctx, std::string const &dir, std::string const &filename)
{
    Model *m = 0;
    try
    {
        if (file->type() == "obj")
        {
            m = new Model(ctx);
            read_obj(file, m, 1.0f, dir);
            return m;
        }
        if (file->type() == "bin")
        {
            m = new Model(ctx);
            read_bin(file, m);
            m->buildMaterials();
            return m;
        }
    }
    catch (...)
    {
        delete m;
        throw;
    }
    throw std::runtime_error("Unknown model file format");
}

Model *Model::createEmpty(GLContext *ctx)
{
    return new Model(ctx);
}

void Model::setVertexData(void const *data, size_t size, uint32_t vBytes)
{
    if (!vertices_)
    {
        glGenBuffers(1, &vertices_);
        glAssertError();
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertices_);
    glAssertError();
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glAssertError();
    vertexBytes_ = vBytes;
    vertexData_.clear();
    vertexData_.insert(vertexData_.end(), (char const *)data, (char const *)data + size);
}

void Model::setIndexData(void const *data, size_t size, uint32_t iBits)
{
    if (!indices_)
    {
        glGenBuffers(1, &indices_);
        glAssertError();
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_);
    glAssertError();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glAssertError();
    indexBits_ = iBits;
    indexCount_ = size / (iBits >> 3);
    indexData_.clear();
    indexData_.insert(indexData_.end(), (char const *)data, (char const *)data + size);
}

void Model::setVertexLayout(VertexLayout const *layout, size_t count)
{
    layout_.swap(std::vector<VertexLayout>(layout, layout + count));
}

void Model::setBatches(TriangleBatch const *batch, size_t count)
{
    batches_.swap(std::vector<TriangleBatch>(batch, batch + count));
}

void Model::setMaterials(Material const *materials, size_t count)
{
    materials_.swap(std::vector<Material>(materials, materials + count));
}

static Vec3 matrixTranslation(float const *m)
{
    return Vec3(m[3], m[7], m[11]);
}

void Model::setBones(Bone const *bones, size_t count)
{
    bones_.swap(std::vector<Bone>(bones, bones + count));
    bool first = true;
    for (size_t i = 0; i != count; ++i)
    {
        if (!equals(bones[i].upperBound, bones[i].lowerBound))
        {
            Vec3 lob(bones[i].lowerBound);
            Matrix bxf;
            get_bone_transform(bones, i, bxf);
            addTo(lob, bxf.translation());
            Vec3 ub(bones[i].upperBound);
            addTo(ub, bxf.translation());
            if (first)
            {
                lower_ = lob;
                upper_ = ub;
                first = false;
            }
            else
            {
                minimize(lower_, lob);
                maximize(upper_, ub);
            }
        }
    }
}


TriangleBatch const *Model::batches(size_t *oCount)
{
    *oCount = batches_.size();
    return *oCount ? &batches_[0] : 0;
}

Material const *Model::materials(size_t *oCount)
{
    *oCount = materials_.size();
    return *oCount ? &materials_[0] : 0;
}

Bone const *Model::bones(size_t *oCount)
{
    *oCount = bones_.size();
    return *oCount ? &bones_[0] : 0;
}

Bone const *Model::boneNamed(std::string const &name)
{
    for (std::vector<Bone>::iterator ptr(bones_.begin()), end(bones_.end());
        ptr != end; ++ptr)
    {
        if (!strncmp((*ptr).name, name.c_str(), sizeof((*ptr).name)))
        {
            return &(*ptr);
        }
    }
    return 0;
}

void Model::buildMaterials()
{
    releaseMaterials();
    for (std::vector<Material>::iterator ptr(materials_.begin()), end(materials_.end());
        ptr != end; ++ptr)
    {
        builtMaterials_.push_back(ctx_->buildMaterial(*ptr));
    }
}

bool Model::hasTransparency() const
{
    for (std::vector<BuiltMaterial *>::const_iterator ptr(builtMaterials_.begin()),
        end(builtMaterials_.end());
        ptr != end;
        ++ptr)
    {
        if ((*ptr)->isTransparent_)
        {
            return true;
        }
    }
    return false;
}

void Model::releaseMaterials()
{
    for (std::vector<BuiltMaterial *>::iterator ptr(builtMaterials_.begin()), end(builtMaterials_.end());
        ptr != end; ++ptr)
    {
        (*ptr)->release();
    }
}

void Model::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, vertices_);
glAssertError();
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
glAssertError();
    for (std::vector<VertexLayout>::iterator ptr(layout_.begin()), end(layout_.end());
        ptr != end; ++ptr)
    {
        switch ((*ptr).channel)
        {
        case vc_position:
            glVertexPointer((*ptr).componentCount, (GLenum)(*ptr).componentKind, vertexBytes_, (void const *)(*ptr).offset);
            glEnableClientState(GL_VERTEX_ARRAY);
            break;
        case vc_normal:
            glNormalPointer((GLenum)(*ptr).componentKind, vertexBytes_, (void const *)(*ptr).offset);
            glEnableClientState(GL_NORMAL_ARRAY);
            break;
        case vc_texture0:
            glTexCoordPointer((*ptr).componentCount, (GLenum)(*ptr).componentKind, vertexBytes_, (void const *)(*ptr).offset);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            break;
        default:
            throw std::runtime_error("Not implemented: other vertex channels");
        }
glAssertError();
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_);
glAssertError();
}

void Model::applyBoneTransform(Bone const *bones, size_t bix, Matrix &m) const
{
    if (bix != 0)
    {
        applyBoneTransform(bones, bones[bix].parent, m);
    }
    multiply(m, *(Matrix const *)bones[bix].xform, m);
}

void Model::issue(Matrix const &modelview, Bone const *bones, bool transparent)
{
glAssertError();
    for (std::vector<TriangleBatch>::iterator ptr(batches_.begin()), end(batches_.end());
        ptr != end; ++ptr)
    {
        BuiltMaterial *bm = builtMaterials_[(*ptr).material];
        if (bm->isTransparent_ == transparent)
        {
            Matrix mv(modelview);
            applyBoneTransform(bones ? bones : &bones_[0], (*ptr).bone, mv);
            glLoadTransposeMatrixf((float *)mv.rows);
            builtMaterials_[(*ptr).material]->apply();
            glDrawRangeElements(GL_TRIANGLES, (*ptr).minVertexIndex, (*ptr).maxVertexIndex, (*ptr).numTriangles * 3, 
                (indexBits_ == 32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
                (void *)((*ptr).firstTriangle * 3 * indexBits_ >> 3));
    glAssertError();
        }
    }
}

void const *Model::vertices() const
{
    if (vertexData_.empty())
    {
        return 0;
    }
    return &vertexData_[0];
}

size_t Model::vertexSize() const
{
    return vertexBytes_;
}

size_t Model::vertexCount() const
{
    return vertexData_.size() / vertexBytes_;
}



void const *Model::indices() const
{
    if (indexData_.empty())
    {
        return 0;
    }
    return &indexData_[0];
}

size_t Model::indexBits() const
{
    return indexBits_;
}

size_t Model::indexCount() const
{
    return indexCount_;
}

Vec3 Model::lowerBound() const
{
    return lower_;
}

Vec3 Model::upperBound() const
{
    return upper_;
}

Model::Model(GLContext *ctx)
{
    ctx_ = ctx;
    vertices_ = 0;
    indices_ = 0;
    vertexBytes_ = 0;
    indexBits_ = 0;
}


ModelWriter::~ModelWriter()
{
    delete wr_;
}

ModelWriter *ModelWriter::writeToFile(IxWrite *file)
{
    ModelWriter *mw = new ModelWriter();
    mw->wr_ = file;
    uint32_t magic = bm_magicFile;
    file->write(&magic, 4);
    return mw;
}

void ModelWriter::setVertexData(void const *data, size_t size, uint32_t vBytes)
{
    uint32_t magic = bm_magicVertices;
    wr_->write(&magic, 4);
    wr_->write(&vBytes, 4);
    writeBuffer(wr_, data, size);
}

void ModelWriter::setIndexData(void const *data, size_t size, uint32_t iBits)
{
    uint32_t magic = bm_magicIndices;
    wr_->write(&magic, 4);
    wr_->write(&iBits, 4);
    writeBuffer(wr_, data, size);
}

void ModelWriter::setVertexLayout(VertexLayout const *layout, size_t count)
{
    uint32_t magic = bm_magicLayout;
    wr_->write(&magic, 4);
    writeBuffer(wr_, layout, count * sizeof(VertexLayout));
}

void ModelWriter::setBatches(TriangleBatch const *batch, size_t count)
{
    uint32_t magic = bm_magicBatches;
    wr_->write(&magic, 4);
    writeBuffer(wr_, batch, count * sizeof(TriangleBatch));
}

void ModelWriter::setMaterials(Material const *materials, size_t count)
{
    uint32_t magic = bm_magicMaterials;
    wr_->write(&magic, 4);
    writeBuffer(wr_, materials, count * sizeof(Material));
}

void ModelWriter::setBones(Bone const *bones, size_t count)
{
    uint32_t magic = bm_magicBones;
    wr_->write(&magic, 4);
    writeBuffer(wr_, bones, count * sizeof(Bone));
}

ModelWriter::ModelWriter()
{
    wr_ = 0;
}

void get_bone_transform(Bone const *skel, size_t boneIndex, Matrix &oMat)
{
    if (boneIndex == 0 || skel[boneIndex].parent == boneIndex)
    {
        oMat = *(Matrix const *)skel[boneIndex].xform;
    }
    else {
        get_bone_transform(skel, skel[boneIndex].parent, oMat);
        multiply(oMat, *(Matrix const *)skel[boneIndex].xform, oMat);
    }
}

