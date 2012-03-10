
#if !defined (rc_vertexmerge_h)
#define rc_vertexmerge_h

#include <stdint.h>

//  I sort a set of vertex data into unique, fully expanded vertices.
//  I do not weld values that are barely equal, only totally equal.
template<typename Vertex>
class VertexMerge
{
public:
    uint32_t addVertex(Vertex const &v)
    {
        uint32_t h = hash(v);
        std::vector<VertexInfo>::iterator ptr(hash_[h & 4095].begin()),
            end(hash_[h & 4095].end());
        uint32_t i = 0;
        while (ptr != end)
        {
            if ((*ptr).hash_ == h && equals(vertices_[(*ptr).index_], v))
            {
                i = (*ptr).index_;
                goto got_ix;
            }
            ++ptr;
        }
        i = vertices_.size();
        vertices_.push_back(v);
        hash_[h & 4095].push_back(VertexInfo(h, i));
    got_ix:
        indices_.push_back(i);
        return i;
    }
    std::vector<Vertex> const &getVertices() const
    {
        return vertices_;
    }
    std::vector<uint32_t> const &getIndices() const
    {
        return indices_;
    }
private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    inline static bool equals(Vertex const &a, Vertex const &b)
        {
            return memcmp(&a, &b, sizeof(Vertex)) == 0;
        }
    inline static uint32_t hash(Vertex const &a)
        {
            uint32_t code = 0x17;
            uint32_t const *d = (uint32_t const *)&a;
            while (d+1 <= (uint32_t const*)(&a + 1))
            {
                code = code * 0x75afed + *d + 0xfeef001;
                ++d;
            }
            return code;
        }
    struct VertexInfo
    {
        VertexInfo(uint32_t h, uint32_t i) :
            hash_(h),
            index_(i)
        {
        }
        uint32_t hash_;
        uint32_t index_;
    };
    std::vector<VertexInfo> hash_[4096];
};


#endif  //  rc_vertexmerge_h
