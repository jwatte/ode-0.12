
#include "rc_obj.h"
#include "rc_math.h"
#include "rc_reference.h"
#include "rc_ixfile.h"
#include "rc_model.h"
#include "rc_vertexmerge.h"

#include <ctype.h>
#include <string.h>

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <list>


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
    Mtl *getMaterial(std::string const &name)
    {
        for (std::vector<Mtl>::iterator ptr(mtls.begin()), end(mtls.end());
            ptr != end; ++ptr)
        {
            if ((*ptr).name == name)
            {
                return &(*ptr);
            }
        }
        return 0;
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

void parse_obj_line(char const *data, char const *eol, Obj &obj, float vScale)
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
        if ((obj.groupmtls.size() > 0 && obj.groupmtls.back().offset + 30000 <= obj.faces.size())
            || (obj.groupmtls.size() == 0 && obj.faces.size() >= 30000))
        {
            //  no more than 10,000 triangles per batch
            obj.groupMtlDirty = true;
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
            s = vScale;
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
        obj.groupMtlDirty = true;
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

void read_obj(IxRead *file, IModelData *m, float vScale, std::string const &dir)
{
    Obj obj(dir);
    char const *data = (char const *)file->dataSegment(0, file->size());
    char const *end = data + file->size();
    while (data < end)
    {
        char const *eol = end_of_line(data, end);
        parse_obj_line(data, eol, obj, vScale);
        data = eol;
        while (data < end && (*data == '\n' || *data == '\r'))
        {
            ++data;
        }
    }
    //  build batches
    std::vector<Bone> bones;
    Bone bone;  //  root bone with empty name
    memcpy(bone.xform, &Matrix::identity.rows[0][0], sizeof(bone.xform));
    bones.push_back(bone);
    std::list<VertexMerge<VertPtn> > boneVerts;

    std::string boneName("!! marker bone name not used in any scene !! $$$$$ 12378168476378");
    VertexMerge<VertPtn> *vm = 0;
    TriangleBatch batch;
    std::vector<TriangleBatch> batches;
    VertexMerge<Material> materials;
    for (size_t i = 0, n = obj.groupmtls.size(); i != n; ++i)
    {
        GroupMtl const &gmtl = obj.groupmtls[i];
        if (boneName != gmtl.groupname)
        {
            boneName = gmtl.groupname;
            boneVerts.push_back(VertexMerge<VertPtn>());
            vm = &boneVerts.back();
            strncpy_s(bone.name, boneName.c_str(), sizeof(bone.name));
            bone.name[sizeof(bone.name)-1] = 0;
            bone.parent = 0;
            memcpy(bone.xform, Matrix::identity.rows[0], sizeof(bone.xform));
            bones.push_back(bone);
            batch.bone = bones.size() - 1;
        }
        size_t top = obj.faces.size();
        if (i < n-1)
        {
            top = obj.groupmtls[i+1].offset;
        }
        batch.firstTriangle = vm->getIndices().size() / 3;
        for (std::vector<int>::iterator ptr(obj.faces.begin() + gmtl.offset), end(obj.faces.begin() + top);
            ptr != end;)
        {
            VertPtn vptn;
            int pi = *ptr++;
            int ti = *ptr++;
            int ni = *ptr++;
            memcpy(&vptn.x, &obj.pos[pi * 3], 12);
            memcpy(&vptn.tu, &obj.uv[ti * 2], 8);
            memcpy(&vptn.nx, &obj.norm[ni * 3], 12);
            vm->addVertex(vptn);
        }
        batch.numTriangles = vm->getIndices().size() / 3 - batch.firstTriangle;
        Material om;
        convert_obj_material(dir, *obj.getMaterial(gmtl.mtlname), om);
        batch.material = materials.addVertex(om);
        batches.push_back(batch);
    }
    std::vector<VertPtn> vertexData;
    std::vector<uint32_t> indexData;
    size_t vOffset = 0;
    size_t iOffset = 0;
    size_t bix = 1;
    for (std::list<VertexMerge<VertPtn> >::iterator ptr(boneVerts.begin()), end(boneVerts.end());
        ptr != end; ++ptr)
    {
        size_t ni = 0, nv = 0;
        if ((*ptr).getIndices().size())
        {
            VertPtn const *vp = &(*ptr).getVertices()[0];
            nv = (*ptr).getVertices().size();
            uint32_t const *ip = &(*ptr).getIndices()[0];
            ni = (*ptr).getIndices().size();
            vertexData.insert(vertexData.end(), vp, vp + nv);
            indexData.insert(indexData.end(), ip, ip + ni);
            for (uint32_t *uip = &indexData[0] + indexData.size() - ni, *eip = uip + ni; uip != eip; ++uip)
            {
                *uip += vOffset;
            }
            //  now, center each group for its "bone" to allow simple animation
            Vec3 vMin(&vp->x), vMax(&vp->x);
            for (VertPtn *vvp = &vertexData[0] + vertexData.size() - nv, *evp = vvp + nv; vvp != evp; ++vvp)
            {
                minimize(vMin, Vec3(&vvp->x));
                maximize(vMax, Vec3(&vvp->x));
            }
            Vec3 lob(vMin), ub(vMax);
            addTo(vMin, vMax);
            scale(vMin, -0.5f);
            for (VertPtn *vvp = &vertexData[0] + vertexData.size() - nv, *evp = vvp + nv; vvp != evp; ++vvp)
            {
                addTo(*(Vec3 *)&vvp->x, vMin);
            }
            scale(vMin, -1);
            (*(Matrix *)bones[bix].xform).setTranslation(vMin);
            bones[bix].lowerBound = lob;
            bones[bix].upperBound = ub;
            vOffset += nv;
            iOffset += ni;
        }
        for (size_t i = 0, n = batches.size(); i != n; ++i)
        {
            if (batches[i].bone == bix)
            {
                batches[i].firstTriangle += (iOffset - ni) / 3;
                batches[i].minVertexIndex = vOffset - nv;
                batches[i].maxVertexIndex = vOffset - 1;
            }
        }
        ++bix;
    }
    for (std::vector<Bone>::iterator ptr(bones.begin()), end(bones.end()); ptr != end; ++ptr)
    {
        std::transform((*ptr).name, &(*ptr).name[strlen((*ptr).name)], (*ptr).name, tolower);
    }
    m->setBones(&bones[0], bones.size());
    m->setMaterials(&materials.getVertices()[0], materials.getVertices().size());
    m->setBatches(&batches[0], batches.size());
    m->setIndexData(&indexData[0], indexData.size() * 4, 32);
    m->setVertexData(&vertexData[0], vertexData.size() * sizeof(VertPtn), sizeof(VertPtn));
    m->setVertexLayout(VertPtn::layout_, sizeof(VertPtn::layout_)/sizeof(VertPtn::layout_[0]));
}

