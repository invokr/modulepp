/**
 * @file module_implementation.hpp
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

#ifndef _MODULEPP_MODULE_IMPLEMENTATION_UNIX_HPP_
#define	_MODULEPP_MODULE_IMPLEMENTATION_UNIX_HPP_
#if defined(__LINUX__) || defined(__APPLE__) || defined(hpux) || defined(_hpux) || defined(__GNUC__)

#include <mutex>
#include <cstddef>
#include <dlfcn.h>

#include "module_library_exceptions.hpp"

namespace modulepp {
    /** Shared library implementation for unix systems */
    class shared_library_unix {
        public:
            /** Library loading flags. */
            enum Flags {
                SHLIB_GLOBAL_IMPL = 1,
                SHLIB_LOCAL_IMPL  = 2
            };

            /** Constructor */
            shared_library_unix() : path(""), handle(nullptr) {}

            /** Destructor */
            virtual ~shared_library_unix() {
                unload(); // free when destructing
            }

            /** Load library from given path */
            void load(const std::string& path, int flags = 0) {
                std::lock_guard<std::mutex> lock(mutex);

                if (handle != nullptr) {
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
                    handle = nullptr;
                    throw libraryLoadException();
                }
            }

            /** Unload library freeing all ressources */
            void unload() {
                std::lock_guard<std::mutex> lock(mutex);

                if (handle != nullptr) {
                    dlclose(handle);
                    handle = nullptr;
                }
            }

            /** Check if library is loaded */
            bool isLoaded() {
                return (handle != nullptr);
            }

            /** Return symbol to looked up pointer or NULL if non is found. */
            void* findSymbol(const std::string& name) {
                std::lock_guard<std::mutex> lock(mutex);

                void* result = nullptr;
                if (handle != nullptr) {
                    result = dlsym(handle, name.c_str());
                } else {
                    throw libraryAccessException();
                }

                return result;
            }

            /** Returns library suffix. */
            const std::string suffix() {
                #if defined(__APPLE__)
                    return ".dylib";
                #elif defined(hpux) || defined(_hpux)
                    return ".sl";
                #else
                    return ".so";
                #endif
            }
        protected:
            /** Path to the library. */
            std::string path;
            /** Pointer to the shared library. */
            void* handle;
            /** Mutex to synchonize library access. */
            mutable std::mutex mutex;
        private:
            /** Prevent copying */
            shared_library_unix(const shared_library_unix&) = delete;
    };
};

#endif  /* defined(__LINUX__) || defined(__APPLE__) || defined(hpux) || defined(_hpux) || defined(__GNUC__) */
#endif	/* _MODULEPP_MODULE_IMPLEMENTATION_UNIX_HPP_ */