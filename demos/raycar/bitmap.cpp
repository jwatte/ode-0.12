
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

static struct {
    std::string ext;
    ILenum fmt;
}
supportedExts[] = {
    { std::string("tga"), IL_TGA },
    { std::string("jpg"), IL_JPG },
    { std::string("png"), IL_PNG },
    { std::string("gif"), IL_GIF },
    { std::string("bmp"), IL_BMP },
    { std::string("tif"), IL_TIF },
    { std::string("jpeg"), IL_JPG },
    { std::string("tiff"), IL_TIF },
};

bool is_supported_bitmap_ext(std::string const &ext)
{
    for (size_t i = 0, n = sizeof(supportedExts)/sizeof(supportedExts[0]); i != n; ++i)
    {
        if (supportedExts[i].ext == ext)
        {
            return true;
        }
    }
    return false;
}

static ILenum get_bitmap_ext_fmt(std::string const &ext)
{
    for (size_t i = 0, n = sizeof(supportedExts)/sizeof(supportedExts[0]); i != n; ++i)
    {
        if (supportedExts[i].ext == ext)
        {
            return supportedExts[i].fmt;
        }
    }
    throw std::runtime_error(std::string("Not a supported image format: ") + ext);
}

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
    size_t bytesPerPixel()
    {
        ilBindImage(img_);
        return ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
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
    ilGetError();
    ILenum ilFormat = IL_RAW;
    ilFormat = get_bitmap_ext_fmt(fmt);
    bitmap_init();
    int img = ilGenImage();
    ilBindImage(img);
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
    ilEnable(IL_CONV_PAL);
    ilLoadL(ilFormat, data, size);
    int err = ilGetError();
    if (err != IL_NO_ERROR)
    {
        ilDeleteImage(img);
        char errStr[256];
        _snprintf_s(errStr, 256, "Error 0x%x", err);
        errStr[255] = 0;
        throw std::runtime_error(std::string("Error loading image format: ") + fmt
            + "\n" + errStr);
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
    ilEnable(IL_FILE_OVERWRITE);
    ilSave(IL_TGA, path.c_str());
    int err = ilGetError();
    if (err != IL_NO_ERROR)
    {
        throw std::runtime_error("Error saving file: " + dst);
    }
}
