#if !defined(rc_model_h)
#define rc_model_h

#include <stdint.h>
#include <vector>

class GLContext;
class IxRead;
class IxWrite;

struct VertexLayout
{
    uint32_t channel;
    uint32_t offset;
    uint32_t componentCount;
    uint32_t componentKind;
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
    mk_endOfKinds
};

enum MapChannel
{
    mk_grayscale,
    mk_alpha,
    mk_rgb,
    mk_rgba,
    mk_endOfChannels
};

struct MapInfo
{
    char name[62];
    MapKind kind;
    MapChannel channel;
};

struct Material
{
    char name[32];
    MapInfo maps[mk_endOfKinds];
};

struct Bone
{
    char name[32];
    uint32_t parent;
    float xform[16];
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
    static Model *readFromFile(IxRead *file, GLContext *ctx);
    static Model *createEmpty(GLContext *ctx);

    void setVertexData(void const *data, size_t size, uint32_t vBytes);
    void setIndexData(void const *data, size_t size, uint32_t iBits);
    void setVertexLayout(VertexLayout const *layout, size_t count);
    void setBatches(TriangleBatch const *batch, size_t count);
    void setMaterials(Material const *materials, size_t count);
    void setBones(Bone const *bones, size_t count);

    TriangleBatch const *batches(size_t *oCount);
    Material const *materials(size_t *oCount);
    Bone const *bones(size_t *oCount);
    void bind();
private:
    Model(GLContext *ctx);

    GLContext *ctx_;
    uint32_t vertices_;
    uint32_t indices_;
    uint32_t vertexBytes_;
    uint32_t indexBits_;
    std::vector<VertexLayout> layout_;
    std::vector<TriangleBatch> batches_;
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


void read_obj(IxRead *file, IModelData *m);
void read_bin(IxRead *file, IModelData *m);


#endif  //  rc_model_h
