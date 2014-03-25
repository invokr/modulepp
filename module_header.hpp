/**
 * @file module_header.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.1
 *
 * @par License
 *    Module++
 *    Copyright 2014 Robin Dietrich
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 * @par License
 *    If available in your jurisdiction, this code may be treated as if placed
 *    in the public domain.
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