
#if !defined(rc_bitmap_h)
#define rc_bitmap_h

#include <string>

class GLContext;
class IxRead;

class Bitmap
{
public:
    virtual void const *bits() = 0;
    virtual size_t size() = 0;
    virtual size_t rowBytes() = 0;
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual ~Bitmap() {}
    static Bitmap *readFromFile(IxRead *file, GLContext *ctx, std::string const &dir, std::string const &name);
};

Bitmap *loadFromData(void const *data, size_t size, std::string const &fmt);
Bitmap *loadFromPath(std::string const &path);

void image_build(std::string const &src, std::string const &dst);
bool is_supported_bitmap_ext(std::string const &ext);

#endif  //  rc_bitmap_h
