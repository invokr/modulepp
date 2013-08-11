#include <iostream>

#include "../module_library.hpp"
#include "module_base.hpp"

using namespace modulepp;

int main() {
    classLoader<module_base> loader;
    loader.loadLibrary("./example_module");
    module_base *b = loader.getClass("module_ext");    
    std::cout << b->getInt() << std::endl;
    
    return 0;   
}