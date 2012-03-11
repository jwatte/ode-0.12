#if !defined(rc_reference_h)
#define rc_reference_h

#include <string>

class IxRead;

class Reference
{
public:
    static void clear();

    //  given an input directory and asset name, open a read stream to that file
    static IxRead *get(std::string const &dir, std::string const &name);
    //  given an input directory and asset name, remember that it needs to be built, 
    //  and return the content root-relative output asset reference (as used at runtime,
    //  not including "content/")
    static std::string build(std::string const &dir, std::string const &name);
    //  given an input asset path, mangle to an output content asset reference 
    //  (including the "content/" prefix)
    static std::string makeOutputName(std::string const &name);

    static size_t numReadItems();
    static std::string const &readItem(size_t i);
    static size_t numBuiltItems();
    static std::string const &builtItem(size_t i);
};

#endif  //  rc_reference_h
