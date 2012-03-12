
#include "rc_asset.h"
#include "rc_model.h"
#include "rc_context.h"
#include "rc_ixfile.h"
#include "rc_bitmap.h"

#include <map>

std::string assetPath("content/");

template<typename T> class AssetLoader
{
public:
    void init(GLContext *ctx)
    {
        ctx_ = ctx;
    }
    void deleteAll()
    {
        for (std::map<std::string, T *>::iterator ptr(loaded_.begin()), end(loaded_.end());
            ptr != end; ++ptr)
        {
            delete (*ptr).second;
        }
    }
    ~AssetLoader()
    {
    }
    T *getOrLoad(std::string const &key)
    {
        std::map<std::string, T *>::iterator ptr(loaded_.find(key));
        if (ptr != loaded_.end())
        {
            return (*ptr).second;
        }
        std::string filePath(assetPath + key);
        T *t = T::readFromFile(IxRead::readFromFile(filePath.c_str()), ctx_, dirname(filePath), filename(filePath));
        loaded_[key] = t;
        return t;
    }
    std::map<std::string, T *> loaded_;
    GLContext *ctx_;
};

class TextAsset
{
public:
    std::string text_;
    char const *text() const
    {
        return text_.c_str();
    }
    static TextAsset *readFromFile(IxRead *rd, GLContext *ctx, std::string const &dir, std::string const &file)
    {
        TextAsset *ret = new TextAsset();
        char const *data = (char const *)rd->dataSegment(0, rd->size());
        ret->text_.insert(ret->text_.end(), data, data + rd->size());
        return ret;
    }
};

AssetLoader<Model> alModel;
AssetLoader<Bitmap> alBitmap;
AssetLoader<TextAsset> alText;

void Asset::clearAllAssets()
{
    alModel.deleteAll();
    alBitmap.deleteAll();
    alText.deleteAll();
}

void Asset::init(GLContext *ctx)
{
    alModel.init(ctx);
    alBitmap.init(ctx);
    alText.init(ctx);
}

Model *Asset::model(std::string const &name)
{
    return alModel.getOrLoad(name);
}

Bitmap *Asset::bitmap(std::string const &name)
{
    return alBitmap.getOrLoad(name);
}

char const *Asset::text(std::string const &name)
{
    return alText.getOrLoad(name)->text();
}
