
#include "rc_bitmap.h"
#include "rc_ixfile.h"

#include <stdexcept>
#include <string>

#include <IL/il.h>


bool inited = false;

static void bitmap_init()
{
    if (inited)
    {
        return;
    }
    ilInit();
    if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
    {
        throw std::runtime_error("Bad DevIL.dll version.");
    }
    inited = true;
}

static std::string dotTga("tga");
static std::string dotPng("png");
static std::string dotJpg("jpg");

class ILBitmap : public Bitmap
{
public:
    ILBitmap(int img) : img_(img)
    {
        width_ = ilGetInteger(IL_IMAGE_WIDTH);
        height_ = ilGetInteger(IL_IMAGE_HEIGHT);
    }
    ~ILBitmap()
    {
        ilDeleteImage(img_);
    }
    size_t width_;
    size_t height_;
    int img_;
    void const *bits()
    {
        ilBindImage(img_);
        return ilGetData();
    }
    size_t size()
    {
        ilBindImage(img_);
        return ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL) * width_ * height_;
    }
    size_t rowBytes()
    {
        ilBindImage(img_);
        return ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL) * width_;
    }
    size_t width()
    {
        return width_;
    }
    size_t height()
    {
        return height_;
    }
};

Bitmap *loadFromData(void const *data, size_t size, std::string const &fmt)
{
    ILenum ilFormat = IL_RAW;
    if (fmt == dotTga)
    {
        ilFormat = IL_TGA;
    }
    else if (fmt == dotPng)
    {
        ilFormat = IL_PNG;
    }
    else if (fmt == dotJpg)
    {
        ilFormat = IL_JPG;
    }
    else
    {
        throw std::runtime_error(std::string("Unknown image format: ") + fmt);
    }
    bitmap_init();
    int img = ilGenImage();
    ilBindImage(img);
    ilLoadL(ilFormat, data, size);
    if (ilGetError() != IL_NO_ERROR)
    {
        ilDeleteImage(img);
        throw std::runtime_error(std::string("Error loading image format: ") + fmt);
    }
    return new ILBitmap(img);
}

Bitmap *loadFromPath(std::string const &path)
{
    bitmap_init();
    Bitmap *ret = NULL;
    IxRead *ir = IxRead::readFromFile(path.c_str());
    try
    {
        ret = loadFromData(ir->dataSegment(0, ir->size()), ir->size(), fileext(path));
    }
    catch (...)
    {
        delete ir;
        throw;
    }
    delete ir;
    return ret;
}

Bitmap *Bitmap::readFromFile(IxRead *file, GLContext *ctx, std::string const &dir, std::string const &filename)
{
    return loadFromData(file->dataSegment(0, file->size()), file->size(), fileext(filename));
}

void image_build(std::string const &src, std::string const &dst)
{
    ILBitmap *bmp = static_cast<ILBitmap *>(loadFromPath(src));
    std::string out(filenoext(dst) + ".tga");
    std::wstring path;
    path.insert(path.end(), out.begin(), out.end());
    ilBindImage(bmp->img_);
    ilSave(IL_TGA, path.c_str());
}
