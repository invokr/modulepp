#include "../module_header.hpp"
#include "module_base.hpp"

class module_ext : public module_base {
    public:
        module_ext() {}
        virtual ~module_ext() {}
        virtual int getInt();
};

BEGIN_MODULE_FACTORY(module_base)
    EXPORT_CLASS(module_ext)
END_MODULE_FACTORY
