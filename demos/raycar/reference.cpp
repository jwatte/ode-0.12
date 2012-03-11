
#include "rc_reference.h"
#include "rc_ixfile.h"

#include <set>
#include <string>
#include <algorithm>


static std::set<std::string> readItems_;
static std::set<std::string> builtItems_;
static std::set<std::string>::iterator readItemsIter_;
static size_t readItemsIterPos_;
static std::set<std::string>::iterator builtItemsIter_;
static size_t builtItemsIterPos_;

void Reference::clear()
{
    readItems_.clear();
    readItemsIter_ = readItems_.begin();
    readItemsIterPos_ = 0;
    builtItems_.clear();
    builtItemsIter_ = builtItems_.begin();
    builtItemsIterPos_ = 0;
}

IxRead *Reference::get(std::string const &dir, std::string const &name)
{
    std::string path(dir);
    path += "/";
    path += name;
#if defined(_WINDOWS)
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
    IxRead *r = IxRead::readFromFile(path.c_str());
    readItems_.insert(path);
    return r;
}

static std::string new_ext(std::string ext)
{
    if (ext == "obj")
    {
        return "bin";
    }
    if (ext == "tga")
    {
        return "tga";
    }
    if (ext == "scn")
    {
        return "scn";
    }
    throw std::runtime_error(std::string("Don't know how to build an output from file extension: ") + ext);
}

static std::string rel_path(std::string const &path)
{
    std::string file;
    if (path.substr(0, 4) == "art/")
    {
        file = path.substr(4);
    }
    else
    {
        //  there must be an "art" folder in the path
        size_t pos = path.find_first_of("/art/");
        if (pos == std::string::npos)
        {
            throw std::runtime_error(std::string("Cannot make a reference to an asset not in an /art/ directory.\n") +
                path);
        }
        file = path.substr(pos + 5);
    }
    std::string ext = fileext(file);
    std::string subExt = new_ext(ext);
    return file.substr(0, file.size() - ext.size()) + subExt;
}

std::string Reference::build(std::string const &dir, std::string const &name)
{
    std::string path(dir);
    path += "/";
    path += name;
#if defined(_WINDOWS)
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
    builtItems_.insert(path);
    return rel_path(path);
}

std::string Reference::makeOutputName(std::string const &name)
{
    std::string path(name);
#if defined(_WINDOWS)
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
    return std::string("content/") + rel_path(path);
}

size_t Reference::numReadItems()
{
    return readItems_.size();
}

std::string const &Reference::readItem(size_t i)
{
    if (i >= readItems_.size())
    {
        throw std::runtime_error("readItem() argument out of bounds");
    }
    if (i < readItemsIterPos_ || readItemsIterPos_ == 0)
    {
        readItemsIterPos_ = 0;
        readItemsIter_ = readItems_.begin();
    }
    while (i > readItemsIterPos_)
    {
        ++readItemsIter_;
        ++readItemsIterPos_;
    }
    return *readItemsIter_;
}

size_t Reference::numBuiltItems()
{
    return builtItems_.size();
}

std::string const &Reference::builtItem(size_t i)
{

    if (i >= builtItems_.size())
    {
        throw std::runtime_error("builtItem() argument out of bounds");
    }
    if (i < builtItemsIterPos_ || builtItemsIterPos_ == 0)
    {
        builtItemsIterPos_ = 0;
        builtItemsIter_ = builtItems_.begin();
    }
    while (i > builtItemsIterPos_)
    {
        ++builtItemsIter_;
        ++builtItemsIterPos_;
    }
    return *builtItemsIter_;
}

