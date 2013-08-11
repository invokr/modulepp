/**
 * This file is part of module++ (https://bitbucket.org/blue-dev/module).
 *
 * @copyright 2013, Robin Dietrich
 * @license Apache 2.0 <http://www.apache.org/licenses/LICENSE-2.0>
 * @license Public Domain <>
 */
 
#ifndef _MODULEPP_MODULE_HEADER_HPP_
#define _MODULEPP_MODULE_HEADER_HPP_

#include <string>
#include <typeinfo>
#include <exception>

#include "module_factory.hpp"

/**
 * Short usage example:
 * 
 * BEGIN_MODULE_FACTORY(MyBaseClass)
 *     EXPORT_CLASS(MyFirstClass)
 *     EXPORT_CLASS(MySecondClass)
 *     ....
 * END_MODULE_FACTORY
 **/
 
namespace modulepp {
    /** exception thrown when object types mismatch */
    class typeMismatchException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Types mismatch when creating object.";
            }
    };
}

extern "C" {
    bool buildFactory(modulepp::factoryBase *modFactory);
    void initializeLibrary();
    void uninitializeLibrary();
} 

#define BEGIN_MODULE_FACTORY(base)                                       \
bool buildFactory(modulepp::factoryBase *modFactoryBase) {               \
    typedef base modBase;                                                \
    typedef modulepp::factory<std::string, modBase> _factory;            \
                                                                         \
    std::string requiredType(typeid(_factory).name());                   \
    std::string actualType(modFactoryBase->typeName());                  \
                                                                         \
    if (requiredType == actualType) {                                    \
        _factory *modFactory = dynamic_cast<_factory*>(modFactoryBase);

#define EXPORT_CLASS(modClass) \
        modFactory->insert(#modClass, new modulepp::factoryCreatorBasic<modBase, modClass>());

#define EXPORT_CLASS_ADVANCE(modClass) \
        modFactory->insert(#modClass, new modulepp::factoryCreatorAdvance<modBase, modClass>());

#define END_MODULE_FACTORY                                           \
        return true;                                                 \
    } else {                                                         \
        throw modulepp::typeMismatchException();                     \
        return false;                                                \
    }                                                                \
}

#endif /* _MODULEPP_MODULE_HEADER_HPP_ */