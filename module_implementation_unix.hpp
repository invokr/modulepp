/**
 * This file is part of module++ (https://bitbucket.org/blue-dev/module).
 *
 * @copyright 2013, Robin Dietrich
 * @license Apache 2.0 <http://www.apache.org/licenses/LICENSE-2.0>
 * @license Public Domain <>
 */
 
#ifndef _MODULEPP_MODULE_IMPLEMENTATION_UNIX_HPP_
#define	_MODULEPP_MODULE_IMPLEMENTATION_UNIX_HPP_

#include <mutex>
#include <cstddef>
#include <dlfcn.h>

#include "module_library_exceptions.hpp"

namespace modulepp {    
    /** shared library implementation on unix systems */
    class sharedLibraryUnix {
        protected:
            /** Path to the library. */
            std::string path;
            /** Pointer to the shared library. */
            void* handle;
            /** Mutex to synchonize library access. */
            mutable std::mutex mutex;
        public:
            /** Library loading flags. */
            enum Flags {
                SHLIB_GLOBAL_IMPL = 1,
                SHLIB_LOCAL_IMPL  = 2  
            };
            
            /** constructor */
            sharedLibraryUnix() : path(""), handle(NULL) {}
            
            /** load library from given path */
            void load(const std::string& path, int flags = 0) {
                std::lock_guard<std::mutex> lock(mutex);
                
                if (handle != NULL) {
                    throw libraryOverwriteException();
                }
                
                this->path = path+this->suffix();
                
                int realFlags = RTLD_LAZY;
                if (flags & SHLIB_LOCAL_IMPL) {
                    realFlags |= RTLD_LOCAL;
                } else {
                    realFlags |= RTLD_GLOBAL;
                }
                
                handle = dlopen(this->path.c_str(), realFlags);
                if (!handle) {
                    handle = NULL;
                    throw libraryLoadException();
                }
            }
            
            /** unload library freeing all ressources */
            void unload() {
                std::lock_guard<std::mutex> lock(mutex);
                
                if (handle != NULL) {
                    dlclose(handle);
                    handle = NULL;
                }
            }
            
            /** check if library is loaded */
            const bool isLoaded() {
                return (handle != NULL);
            }
            
            /** return symbol to looked up pointer or NULL if non is found. */
            void* findSymbol(const std::string& name) {
                std::lock_guard<std::mutex> lock(mutex);

                void* result = NULL;
                if (handle != NULL) {
                    result = dlsym(handle, name.c_str());
                } else {
                    throw libraryAccessException();
                }
                
                return result;
            }
            
            /** returns library suffix. */
            const std::string suffix() {
                #if defined(__APPLE__)
                    return ".dylib";
                #elif defined(hpux) || defined(_hpux)
                    return ".sl";
                #else
                    return ".so";
                #endif
            }
    };
};

#endif	/* _MODULEPP_MODULE_IMPLEMENTATION_UNIX_HPP_ */