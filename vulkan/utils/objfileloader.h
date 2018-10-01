#include "meshdescription.h"

class ObjFileLoader
{
public:
    static bool read(const std::string& filename, MeshDescription& meshDesc);
};
