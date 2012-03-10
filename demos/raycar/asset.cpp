
#include "rc_asset.h"
#include "rc_model.h"
#include "rc_context.h"
#include "rc_ixfile.h"

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
        T *t = T::readFromFile(IxRead::readFromFile(filePath.c_str()), ctx_, dirname(filePath));
        loaded_[key] = t;
        return t;
    }
    std::map<std::string, T *> loaded_;
    GLContext *ctx_;
};

AssetLoader<Model> alModel;

void Asset::clearAllAssets()
{
    alModel.deleteAll();
}

void Asset::init(GLContext *ctx)
{
    alModel.init(ctx);
}

Model *Asset::model(std::string const &name)
{
    return alModel.getOrLoad(name);
}

