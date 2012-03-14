
#if !defined(rc_obj_h)
#define rc_obj_h

#include <string>

class IxRead;
class IModelData;

void read_obj(IxRead *file, IModelData *m, float scale, std::string const &dir);



#endif  //  rc_obj_h
