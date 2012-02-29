
#include <GL/glew.h>
#include <stdio.h>
#include <string>
#include <ctype.h>

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

char const *end_of_line(char const *pos, char const *end)
{
    while (pos < end)
    {
        if (*pos == '\r' || *pos == '\n')
        {
            break;
        }
        ++pos;
    }
    //  a CRLF (or two LF, or whatever) count as a single line
    if (pos < end && (*pos == '\r' || *pos == '\n'))
    {
        ++pos;
    }
    return pos;
}

class Obj
{
public:
    std::vector<float> pos;
    std::vector<float> norm;
    std::vector<float> uv;
    std::vector<int> faces;
};

bool get_line_float(char const *&line, float &fv)
{
    while (*line && isspace(*line))
    {
        ++line;
    }
    char *end = 0;
    fv = (float)strtod(line, &end);
    if (end != 0 && end > line)
    {
        line = end;
        return true;
    }
    return false;
}

bool get_line_indices(char const *&line, int &a, int &b, int &c)
{
    a = b = c = 0;
    while (*line && isspace(*line))
    {
        ++line;
    }
    ..
}

void parse_obj_line(char const *data, char const *eol, Obj &obj)
{
    while (data < eol && isspace(*data))
    {
        ++data;
    }
    char const *end = data;
    while (end < eol && *end != '\r' && *end != '\n')
    {
        ++end;
    }
    std::string line(data, end);
    if (line.size() < 3)
    {
        throw std::runtime_error("Bad line in obj file");
    }
    if (line[0] == 'f')
    {
        size_t pos = 1;
        if (line[1] == 'o')
        {
            //  face "outline"
            pos = 2;
        }
        while (pos < line.size() && isspace(line[pos]))
        {
            ++pos;
        }
        char const *strptr = line.c_str() + pos;
        int nRefs = 0;
        int a, b, c;
        int abc[6];
        //  turn multi-triangle faces into fans
        while (get_line_indices(strptr, a, b, c))
        {
            if (nRefs >= 2)
            {
                obj.faces.insert(obj.faces.end(), &abc[0], &abc[6]);
                obj.faces.push_back(a);
                obj.faces.push_back(b);
                obj.faces.push_back(c);
            }
            if (nRefs == 0)
            {
                abc[0] = a;
                abc[1] = b;
                abc[2] = c;
            }
            else
            {
                abc[3] = a;
                abc[4] = b;
                abc[5] = c;
            }
            nRefs += 1;
        }
        if (nRefs < 3)
        {
            throw std::runtime_error("Bad obj file: face with less than 3 triangles");
        }
    }
    else if (line[0] == 'v')
    {
        size_t n = 3;
        size_t pos = 1;
        std::vector<float> *vp = &obj.pos;
        if (line[1] == 't')
        {
            vp = &obj.uv;
            n = 2;
            pos = 2;
        }
        else if (line[1] == 'n')
        {
            pos = 2;
            vp = &obj.norm;
        }
        float fv;
        size_t got = 0;
        char const *strptr = line.c_str() + pos;
        while (get_line_float(strptr, fv) && got < n)
        {
            vp->push_back(fv);
            ++got;
        }
        while (got < n)
        {
            vp->push_back(0);
            ++got;
        }
    }
    else
    {
        //  do nothing
    }
}

void read_obj(IxRead *file, IModelData *m)
{
    Obj obj;
    char const *data = (char const *)file->dataSegment(0, file->size());
    char const *end = data + file->size();
    while (data < end)
    {
        char const *eol = end_of_line(data, end);
        parse_obj_line(data, eol, obj);
        data = eol + 1;
    }
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
        default:
            throw std::runtime_error("Unknown bin model file chunk type");
        }
    }
    delete file;
}

Model::~Model()
{
    glDeleteBuffers(1, &vertices_);
    glDeleteBuffers(1, &indices_);
}

Model *Model::readFromFile(IxRead *file, GLContext *ctx)
{
    Model *m = 0;
    try
    {
        if (file->type() == "obj")
        {
            m = new Model(ctx);
            read_obj(file, m);
            return m;
        }
        if (file->type() == "bin")
        {
            m = new Model(ctx);
            read_bin(file, m);
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

void Model::setBones(Bone const *bones, size_t count)
{
    bones_.swap(std::vector<Bone>(bones, bones + count));
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

void Model::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, vertices_);
    glAssertError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_);
    glAssertError();
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

