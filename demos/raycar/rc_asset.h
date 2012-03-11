#if !defined(rc_asset_h)
#define rc_asset_h

#include <string>

class GLContext;
class Model;
class Bitmap;

class Asset
{
public:
    static void clearAllAssets();
    static void init(GLContext *ctx);
    static Model *model(std::string const &name);
    static Bitmap *bitmap(std::string const &name);
};

#endif  //  rc_asset_h
