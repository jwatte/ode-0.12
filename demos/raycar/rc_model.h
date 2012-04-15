#if !defined(rc_model_h)
#define rc_model_h

#include <stdint.h>
#include <vector>

#include "rc_math.h"

class GLContext;
class IxRead;
class IxWrite;
class BuiltMaterial;

enum VertexChannel
{
    vc_position,
    vc_normal,
    vc_color0,
    vc_color1,
    vc_texture0,
    vc_texture1,
    vc_tangent0,
    vc_tangent1,
    vc_bitangent0,
    vc_bitangent1
};

struct VertexLayout
{
    uint32_t channel;
    uint32_t offset;
    uint32_t componentCount;
    uint32_t componentKind;
};

struct VertPtn
{
    float x, y, z;
    float tu, tv;
    float nx, ny, nz;
    static VertexLayout layout_[3];
};

struct TriangleBatch
{
    uint32_t firstTriangle;
    uint32_t numTriangles;
    uint32_t minVertexIndex;
    uint32_t maxVertexIndex;
    uint32_t material;
    uint32_t bone;
};

enum MapKind
{
    mk_ambient,
    mk_diffuse,
    mk_specular,
    mk_opacity,
    mk_normal,
    mk_emissive,
    mk_endOfKinds
};

enum MapChannel
{
    mc_grayscale,
    mc_alpha,
    mc_rgb,
    mc_rgba,
    mc_endOfChannels
};

struct MapInfo
{
    MapInfo() { memset(this, 0, sizeof(*this)); }
    char name[64];
    MapKind kind;
    MapChannel channel;
};

struct Material
{
    Material() { memset(this, 0, sizeof(*this)); }
    MapInfo maps[mk_endOfKinds];
    Rgba colors[mk_endOfKinds];
    float specPower;
};

struct Bone
{
    Bone() { memset(this, 0, sizeof(*this)); }
    char name[32];
    uint32_t parent;
    float xform[16];
    Vec3 lowerBound;
    Vec3 upperBound;
};

class IModelData
{
public:
    virtual ~IModelData() {}
    virtual void setVertexData(void const *data, size_t size, uint32_t vBytes) = 0;
    virtual void setIndexData(void const *data, size_t size, uint32_t iBits) = 0;
    virtual void setVertexLayout(VertexLayout const *layout, size_t count) = 0;
    virtual void setBatches(TriangleBatch const *batch, size_t count) = 0;
    virtual void setMaterials(Material const *materials, size_t count) = 0;
    virtual void setBones(Bone const *bones, size_t count) = 0;
};

class Model : public IModelData
{
public:
    ~Model();
    static Model *readFromFile(IxRead *file, GLContext *ctx, std::string const &dir, std::string const &name);
    static Model *createEmpty(GLContext *ctx);

    void setVertexData(void const *data, size_t size, uint32_t vBytes);
    void setIndexData(void const *data, size_t size, uint32_t iBits);
    void setVertexLayout(VertexLayout const *layout, size_t count);
    void setBatches(TriangleBatch const *batch, size_t count);
    void setMaterials(Material const *materials, size_t count);
    void setBones(Bone const *bones, size_t count);

    void buildMaterials();
    bool hasTransparency() const;

    TriangleBatch const *batches(size_t *oCount);
    Material const *materials(size_t *oCount);
    //  todo: it's not good enough to return Bones as the interface for sub-Models,
    //  because sub-Models may need to be transparency sorted.
    Bone const *bones(size_t *oCount);
    Bone const *boneNamed(std::string const &name);
    void bind();
    void issue(Matrix const &modelview, Bone const *bones = 0, bool transparent = false);

    void const *vertices() const;
    size_t vertexSize() const;
    size_t vertexCount() const;
    void const *indices() const;
    size_t indexBits() const;
    size_t indexCount() const;
    Vec3 lowerBound() const;
    Vec3 upperBound() const;

private:
    friend class ModelSceneNode;
    Model(GLContext *ctx);
    void releaseMaterials();
    void applyBoneTransform(Bone const *bones, size_t bix, Matrix &m) const;

    GLContext *ctx_;
    uint32_t vertices_;
    uint32_t indices_;
    uint32_t vertexBytes_;
    uint32_t indexBits_;
    uint32_t indexCount_;
    Vec3 lower_;
    Vec3 upper_;
    std::vector<char> vertexData_;
    std::vector<char> indexData_;
    std::vector<VertexLayout> layout_;
    std::vector<TriangleBatch> batches_;
    std::vector<BuiltMaterial *> builtMaterials_;
    std::vector<Material> materials_;
    std::vector<Bone> bones_;
};

class ModelWriter : public IModelData
{
public:
    ~ModelWriter();
    static ModelWriter *writeToFile(IxWrite *file);

    void setVertexData(void const *data, size_t size, uint32_t vBytes);
    void setIndexData(void const *data, size_t size, uint32_t iBits);
    void setVertexLayout(VertexLayout const *layout, size_t count);
    void setBatches(TriangleBatch const *batch, size_t count);
    void setMaterials(Material const *materials, size_t count);
    void setBones(Bone const *bones, size_t count);
private:
    ModelWriter();
    IxWrite *wr_;
};

void read_obj(IxRead *file, IModelData *m, float scale, std::string const &dir);
void read_bin(IxRead *file, IModelData *m);

void get_bone_transform(Bone const *skel, size_t boneIndex, Matrix &oMat);

#endif  //  rc_model_h
