#include <iostream>

#include "../module_library.hpp"
#include "module_base.hpp"

using namespace modulepp;

int main() {
    class_loader<module_base> loader;
    loader.load("./example_module");
    module_base *b = loader.create("module_ext");
    std::cout << b->getInt() << std::endl;
    delete b;

    return 0;
}