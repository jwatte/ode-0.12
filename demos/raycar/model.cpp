
#include <GL/glew.h>
#include <stdio.h>
#include <string>
#include <ctype.h>
#include <sstream>
#include <algorithm>
#include <map>

#include "rc_model.h"
#include "rc_context.h"
#include "rc_ixfile.h"
#include "rc_vertexmerge.h"
#include "rc_reference.h"


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
    return pos;
}

char const *skip_space(char const *pos, char const *end = 0)
{
    //  not at end?
    while (((end == 0 && *pos) || (end != 0 && pos != end)) && isspace(*pos))
    {
        ++pos;
    }
    return pos;
}

struct GroupMtl
{
    GroupMtl() : offset(0) {}
    std::string groupname;
    std::string mtlname;
    size_t offset;
};

struct Mtl
{
    Mtl() :
        name(""),
        Ns(20),
        Ni(1.5f),
        d(1.0f),
        Tr(0.0f),
        Tf(1.0f, 1.0f, 1.0f),
        illum(2),
        Ka(0.5f, 0.5f, 0.5f),
        Kd(0.5f, 0.5f, 0.5f),
        Ks(0.5f, 0.5f, 0.5f),
        Ke(0, 0, 0),
        map_Ka(""),
        map_Kd(""),
        map_Ks(""),
        map_Ke("")
    {
    }
    std::string name;
    float Ns;
    float Ni;
    float d;
    float Tr;
    Rgba Tf;
    int illum;
    Rgba Ka;
    Rgba Kd;
    Rgba Ks;
    Rgba Ke;
    std::string map_Ka;
    std::string map_Kd;
    std::string map_Ks;
    std::string map_Ke;
};

class Obj
{
public:
    Obj(std::string const &dir) :
        dir_(dir),
        groupMtlDirty(true)
    {
        //  index 0 is unused in obj, so pre-populate 
        //  that with a default value
        pos.push_back(0);
        pos.push_back(0);
        pos.push_back(0);

        norm.push_back(1);
        norm.push_back(0);
        norm.push_back(0);

        uv.push_back(0);
        uv.push_back(0);

        curGroupMtl.groupname = "default";
        curGroupMtl.mtlname = "default";
    }
    std::vector<float> pos;
    std::vector<float> norm;
    std::vector<float> uv;
    std::vector<int> faces;
    std::string dir_;
    std::vector<GroupMtl> groupmtls;
    GroupMtl curGroupMtl;
    bool groupMtlDirty;
    Mtl curMtl;
    std::vector<Mtl> mtls;
};

bool get_line_float(char const *&line, float &fv)
{
    line = skip_space(line);
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
    line = skip_space(line);
    if (!*line)
    {
        return false;
    }
    char const *start = line;
    while (*line && *line != '/' && *line != ' ')
    {
        ++line;
    }
    if (line - start > 10 || line == start)
    {
        throw std::runtime_error("Invalid vertex index in obj file");
    }
    char *x = 0;
    a = (int)strtol(start, &x, 10);
    if (a == 0 || x != line)
    {
        throw std::runtime_error("Bad vertex index format in obj file");
    }
    if (*line == ' ' || !*line)
    {
        return true;
    }
    ++line;
    start = line;
    while (*line && *line != '/' && *line != ' ')
    {
        ++line;
    }
    if (line - start > 10)
    {
        throw std::runtime_error("Invalid vertex texture in obj file");
    }
    if (line > start)
    {
        char *x = 0;
        b = (int)strtol(start, &x, 10);
        if (b == 0 || x != line)
        {
            throw std::runtime_error("Bad texture index format in obj file");
        }
    }
    if (*line == ' ' || !*line)
    {
        return true;
    }
    ++line;
    start = line;
    while (*line && *line != '/' && *line != ' ')
    {
        ++line;
    }
    if (line - start > 10)
    {
        throw std::runtime_error("Invalid vertex normal in obj file");
    }
    if (line > start)
    {
        char *x = 0;
        c = (int)strtol(start, &x, 10);
        if (c == 0 || x != line)
        {
            throw std::runtime_error("Bad normal index format in obj file");
        }
    }
    if (*line == ' ' || !*line)
    {
        return true;
    }
    throw std::runtime_error("Unexpected data after normal index in obj file");
    return false;
}

void convert(std::string const &str, float &f)
{
    if (1 != sscanf_s(str.c_str(), " %f", &f))
    {
        throw std::runtime_error(std::string("Bad format for obj material float value ") + str);
    }
}

void convert(std::string const &str, int &i)
{
    if (1 != sscanf_s(str.c_str(), " %d", &i))
    {
        throw std::runtime_error(std::string("Bad format for obj material int value ") + str);
    }
}

void convert(std::string const &str, Rgba &c)
{
    if (3 > sscanf_s(str.c_str(), " %f %f %f %f", &c.r, &c.g, &c.b, &c.a))
    {
        throw std::runtime_error(std::string("Bad format for obj material Rgba value ") + str);
    }
}

void convert(std::string const &str, std::string &s)
{
    s = str;
#if defined(_WINDOWS)
    std::replace(s.begin(), s.end(), '\\', '/');
#endif
    size_t pos = s.find_last_of('/');
    if (pos != std::string::npos)
    {
        s = s.substr(pos+1);
    }
}

template<typename T>
void test_mtl(std::string const &line, std::string const &key, T &val)
{
    std::string test(line.substr(0, key.size()));
    if (test == key)
    {
        size_t size = line.size();
        size_t pos = key.size();
        while (pos < size && isspace(line[pos]))
        {
            ++pos;
        }
        convert(line.substr(pos), val);
    }
}

void read_mtllib_line(Obj &obj, std::string const &line)
{
    if (line.substr(0, 7) == "newmtl ")
    {
        if (obj.curMtl.name.size())
        {
            obj.mtls.push_back(obj.curMtl);
        }
        obj.curMtl = Mtl();
        obj.curMtl.name = line.substr(7);
    }
    else
    {
    /*
newmtl 01___Default
	Ns 33.0000
	Ni 1.5000
	d 1.0000
	Tr 0.0000
	Tf 1.0000 1.0000 1.0000 
	illum 2
	Ka 0.5882 0.5882 0.5882
	Kd 0.5882 0.5882 0.5882
	Ks 0.5940 0.5940 0.5940
	Ke 0.0000 0.0000 0.0000
	map_Ka C:\code\ode-0.12\demos\raycar\art\ChassisCompleteMap.tga
	map_Kd C:\code\ode-0.12\demos\raycar\art\raycar.tga
    */
        test_mtl(line, "Ns", obj.curMtl.Ns);
        test_mtl(line, "Ni", obj.curMtl.Ni);
        test_mtl(line, "d", obj.curMtl.d);
        test_mtl(line, "Tr", obj.curMtl.Tr);
        test_mtl(line, "Tf", obj.curMtl.Tf);
        test_mtl(line, "illum", obj.curMtl.illum);
        test_mtl(line, "Ka", obj.curMtl.Ka);
        test_mtl(line, "Kd", obj.curMtl.Kd);
        test_mtl(line, "Ks", obj.curMtl.Ks);
        test_mtl(line, "Ke", obj.curMtl.Ke);
        test_mtl(line, "map_Ka", obj.curMtl.map_Ka);
        test_mtl(line, "map_Kd", obj.curMtl.map_Kd);
        test_mtl(line, "map_Ks", obj.curMtl.map_Ks);
        test_mtl(line, "map_Ke", obj.curMtl.map_Ke);
    }
}

void read_mtllib(std::string const &name, Obj &obj)
{
    IxRead *r = Reference::get(obj.dir_, name);
    size_t size = r->size();
    char const *ptr = (char const *)r->dataSegment(0, size);
    char const *fileEnd = ptr + size;
    while (ptr < fileEnd)
    {
        ptr = skip_space(ptr, fileEnd);
        if (ptr == fileEnd)
        {
            break;
        }
        char const *end = end_of_line(ptr, fileEnd);
        std::string line(ptr, end);
        ptr = end;
        while (ptr < fileEnd && (*ptr == '\n' || *ptr == '\r'))
        {
            ++ptr;
        }
        read_mtllib_line(obj, line);
    }
    if (obj.curMtl.name.size())
    {
        obj.mtls.push_back(obj.curMtl);
    }
    delete r;
}

void parse_obj_line(char const *data, char const *eol, Obj &obj, float scale)
{
    data = skip_space(data, eol);
    if (data == eol || *data == '\r' || *data == '\n')
    {
        //  empty line -- nothing to do
        return;
    }
    char const *end = end_of_line(data, eol);
    if (end - data < 3)
    {
        //  not a line I care about -- make sure I don't index past end of line
        return;
    }
    std::string line(data, end);
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
        if (obj.groupMtlDirty)
        {
            obj.groupMtlDirty = false;
            obj.curGroupMtl.offset = obj.faces.size();
            obj.groupmtls.push_back(obj.curGroupMtl);
        }
        char const *strptr = line.c_str() + pos;
        int nRefs = 0;
        int a, b, c;
        int abc[6];
        //  turn multi-triangle faces into fans
        while (get_line_indices(strptr, a, b, c))
        {
            //  translate negative indices, and zero indices
            if (a < 0)
            {
                a = obj.pos.size() + a;
            }
            if (b < 0)
            {
                b = obj.uv.size() + b;
            }
            if (c < 0)
            {
                c = obj.norm.size() + c;
            }
            if ((uint32_t)a * 3 > obj.pos.size())
            {
                throw std::runtime_error("Position index out of range in obj file.");
            }
            if ((uint32_t)b * 2 > obj.uv.size())
            {
                throw std::runtime_error("Texture index out of range in obj file.");
            }
            if ((uint32_t)c * 3 > obj.norm.size())
            {
                throw std::runtime_error("Normal index out of range in obj file.");
            }
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
            throw std::runtime_error("Bad obj file: face with less than 3 vertices");
        }
    }
    else if (line[0] == 'v')
    {
        float s = 1.0f;
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
        else
        {
            s = scale;
        }
        float fv;
        size_t got = 0;
        char const *strptr = line.c_str() + pos;
        while (get_line_float(strptr, fv) && got < n)
        {
            vp->push_back(fv * s);
            ++got;
        }
        while (got < n)
        {
            vp->push_back(0);
            ++got;
        }
    }
    else if (line.substr(0, 7) == "mtllib ")
    {
        size_t n = 7;
        size_t size = line.size();
        while (n < size && isspace(line[n]))
        {
            ++n;
        }
        if (n == size)
        {
            throw std::runtime_error("Missing material library file name in obj");
        }
        std::string name(line.substr(n));
        read_mtllib(name, obj);
    }
    else if (line.substr(0, 7) == "usemtl ")
    {
        obj.groupMtlDirty = true;
        size_t size = line.size(), pos = 7;
        while (pos < size && line[pos] == ' ')
        {
            ++pos;
        }
        if (pos == line.size())
        {
            throw std::runtime_error("Missing material name in usemtl in obj");
        }
        std::string name(line.substr(pos));
        obj.curGroupMtl.mtlname = name;
    }
    else if (line[0] == 'g')
    {
        size_t pos = 1, size = line.size();
        while (pos < size && isspace(line[pos]))
        {
            ++pos;
        }
        if (pos == size)
        {
            throw std::runtime_error("Missing group name in obj file");
        }
        std::string name(line.substr(pos));
        obj.curGroupMtl.groupname = name;
    }
    else
    {
        //  do nothing
    }
}

void convert_mapname(std::string const &dir, std::string const &mapname, MapInfo &mi, MapKind mk)
{
    memset(&mi, 0, sizeof(mi));
    if (!mapname.size())
    {
        return;
    }
    std::string outName(Reference::build(dir, mapname));
    if (outName.size() >= sizeof(mi.name))
    {
        throw std::runtime_error(std::string("Map path name is too long: ") + outName);
    }
    mi.channel = mc_rgb;
    mi.kind = mk;
    strncpy_s(mi.name, outName.c_str(), sizeof(mi.name));
    mi.name[sizeof(mi.name)-1] = 0;
}

void convert_obj_material(std::string const &dir, Mtl const &om, Material &mm)
{
    memset(&mm, 0, sizeof(mm));
    mm.specPower = om.Ns;
    mm.colors[mk_ambient] = om.Ka;
    mm.colors[mk_diffuse] = om.Kd;
    mm.colors[mk_specular] = om.Ks;
    mm.colors[mk_emissive] = om.Ke;
    convert_mapname(dir, om.map_Ka, mm.maps[mk_ambient], mk_ambient);
    convert_mapname(dir, om.map_Kd, mm.maps[mk_diffuse], mk_diffuse);
    convert_mapname(dir, om.map_Ks, mm.maps[mk_specular], mk_specular);
    convert_mapname(dir, om.map_Ke, mm.maps[mk_emissive], mk_emissive);
}

void read_obj(IxRead *file, IModelData *m, float scale, std::string const &dir)
{
    Obj obj(dir);
    char const *data = (char const *)file->dataSegment(0, file->size());
    char const *end = data + file->size();
    while (data < end)
    {
        char const *eol = end_of_line(data, end);
        parse_obj_line(data, eol, obj, scale);
        data = eol;
        while (data < end && (*data == '\n' || *data == '\r'))
        {
            ++data;
        }
    }
    //  now, normalize p, t, n triplets into individual vertex references
    VertexMerge<VertPtn> merge;
    for (std::vector<int>::iterator ptr(obj.faces.begin()), end(obj.faces.end());
        ptr != end;)
    {
        VertPtn vptn;
        int pi = *ptr++;
        int ti = *ptr++;
        int ni = *ptr++;
        memcpy(&vptn.x, &obj.pos[pi * 3], 12);
        memcpy(&vptn.tu, &obj.uv[ti * 2], 8);
        memcpy(&vptn.nx, &obj.norm[ni * 3], 12);
        merge.addVertex(vptn);
    }
    //  build batches
    std::vector<TriangleBatch> batches;
    VertexMerge<Material> materials;
    TriangleBatch batch;
    for (size_t i = 0, n = obj.groupmtls.size(); i != n; ++i)
    {
        std::vector<uint32_t> const &idcs = merge.getIndices();
        size_t end = idcs.size();
        if (i < n-1)
        {
            //  p, t, n makes 3 turn into one index
            end = obj.groupmtls[i+1].offset / 3;
        }
        //  p, t, n makes 3 turn into one index, divide by 3 to get triangles
        batch.firstTriangle = obj.groupmtls[i].offset / 9;
        batch.numTriangles = end / 3 - batch.firstTriangle;
        batch.minVertexIndex = idcs[batch.firstTriangle * 3];
        batch.maxVertexIndex = batch.minVertexIndex;
        for (size_t q = batch.firstTriangle * 3, qn = q + batch.numTriangles * 3; q != qn; ++q)
        {
            uint32_t ix = idcs[q];
            if (ix < batch.minVertexIndex)
            {
                batch.minVertexIndex = ix;
            }
            if (ix > batch.maxVertexIndex)
            {
                batch.maxVertexIndex = ix;
            }
        }
        batch.bone = 0;
    //  build materials
        uint32_t mtlix = obj.mtls.size();
        for (size_t m = 0, mn = obj.mtls.size(); m != mn; ++m)
        {
            if (obj.groupmtls[i].mtlname == obj.mtls[m].name)
            {
                mtlix = m;
            }
        }
        if (mtlix == obj.mtls.size())
        {
            throw std::runtime_error(std::string("Used material name is not in library: ") + obj.groupmtls[i].mtlname);
        }
        Material mm;
        convert_obj_material(obj.dir_, obj.mtls[mtlix], mm);
        batch.material = materials.addVertex(mm);
        batches.push_back(batch);
    }
    if (batch.firstTriangle + batch.numTriangles != merge.getIndices().size() / 3)
    {
        throw std::runtime_error("internal triangle count check failed for obj");
    }
    m->setMaterials(&materials.getVertices()[0], materials.getVertices().size());
    m->setBatches(&batches[0], batches.size());
    m->setIndexData(&merge.getIndices()[0], merge.getIndices().size() * 4, 32);
    m->setVertexData(&merge.getVertices()[0], merge.getVertices().size() * sizeof(VertPtn), sizeof(VertPtn));
    m->setVertexLayout(VertPtn::layout_, sizeof(VertPtn::layout_)/sizeof(VertPtn::layout_[0]));
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

void Model::issue(Matrix const &modelview)
{
    glLoadTransposeMatrixf((float *)modelview.rows);
glAssertError();
    for (std::vector<TriangleBatch>::iterator ptr(batches_.begin()), end(batches_.end());
        ptr != end; ++ptr)
    {
        builtMaterials_[(*ptr).material]->apply();
        glDrawRangeElements(GL_TRIANGLES, (*ptr).minVertexIndex, (*ptr).maxVertexIndex, (*ptr).numTriangles * 3, 
            (indexBits_ == 32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            (void *)((*ptr).firstTriangle * 3 * indexBits_ >> 3));
glAssertError();
    }
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

